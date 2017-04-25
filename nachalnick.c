#define _GNU_SOURCE
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FORMAT_DT "dd.mm.yyyy"
#define FORMAT_TM "HH:MM"
#define FORMAT_DT_SIZE 11
#define FORMAT_TM_SIZE 6
char CONF_PATH[BUFSIZ];

/* Zone of interest */
/* Command for invoking notification */
#define NOTIF_COM "notify-send"
/* Config file name */
#define CONF_FILE "nach.list"
/* Temporary file full path */
#define CONF_TMP_FILE "/tmp/nach.list.back"
/* Default refresh rate in seconds, default is 1200, i.e. 20min */
unsigned REFRESH_RATE = 1200;

struct entry
{
	int type;
	struct tm et;
	char text[BUFSIZ];
};

void
set_path()
{
	const char * homedir;
	/* man pwd.h for more info */
	if ( (homedir = getenv("XDG_CONFIG_HOME")) == NULL )
	{
		if ( (homedir = getenv("HOME")) == NULL )
			homedir = getpwuid(getuid())->pw_dir;

		strcpy( CONF_PATH, homedir );
		strcat( CONF_PATH, "/.config/" );
	}
	else
	{
		strcpy( CONF_PATH, homedir );
		strcat( CONF_PATH, "/" );
	}

	strcat( CONF_PATH, CONF_FILE );
}

void
add_task( int argc, char ** argv )
{
	int in_type = 0;
	char in_text[ BUFSIZ ];
	char in_date[ FORMAT_DT_SIZE ];
	char in_time[ FORMAT_TM_SIZE ];
	short i;
	FILE * config;

	for ( i = 1; i < argc; ++i )
	{
		if ( argv[i][0] == '-' && argv[i][1] == 'a' )
		{
			in_type = atoi( argv[i+1] );
			if ( in_type != 0 )
			{
				strncpy( in_date, argv[i+2], FORMAT_DT_SIZE );
				strncpy( in_time, argv[i+3], FORMAT_TM_SIZE );
				strncpy( in_text, argv[i+4], BUFSIZ );
			}
			else
			{
				/* type 0 uses current time */
				time_t current_time = time(NULL);
				struct tm * tstr = localtime( &current_time );
				sprintf( in_date, "%d.%d.%d", tstr->tm_mday, tstr->tm_mon + 1, tstr->tm_year + 1900 );
				sprintf( in_time, "%d:%d", tstr->tm_hour, tstr->tm_min );
				strncpy( in_text, argv[i+2], BUFSIZ );
			}

		}
	}

	config = fopen( CONF_PATH, "a" );
	fprintf( config, "%i %s %s\n%s\n", in_type, in_date, in_time, in_text );
	fclose( config );
}

void
help_print( void )
{
	puts("Usage: nachalnick [OPTIONS]");
	puts("--------");
	puts("Options:");
	puts("  Add entry:");
	printf("\t-a TYPE [%s %s] \"TEXT\"\n", FORMAT_DT, FORMAT_TM);
	puts("\tTYPE 0 means constant reminding and nees only text");
	puts("\tTYPE 1 means delayed reminding and needs time and date before text");
	puts("\n  List entries:");
	puts("\t-L");
	puts("\n  Remove entry #NUMBER:");
	puts("\t-r NUMBER");
	puts("\n  Set refresh rate. Set before -d. If not used, refresh will happen every 20mins:");
	puts("\t-t SECONDS");
	puts("\n  Start daemon:");
	puts("\t-d &");
}

unsigned
list_print( void )
{
	FILE * config;
	char bufline[BUFSIZ];
	int err;
	unsigned lines;

	config = fopen( CONF_PATH, "r" );
	err = errno;
	if ( err == ENOENT )
	{
		puts( "Config file does not exist" );
		return 0;
	}

	lines = 0;
	while( feof(config) == 0 )
		if ( fgets( bufline, BUFSIZ, config ) != NULL )
			++lines;

	if ( lines > 0 )
	{
		unsigned i;
		rewind( config );
		for ( i = 1; i <= lines; ++i )
			if ( fgets( bufline, BUFSIZ, config ) != NULL )
				if ( i % 2 == 0 )
					printf ( "%u. %s", i / 2, bufline );
	}

	fclose( config );
	return lines / 2;
}

void
delete_entry( int num )
{
	unsigned entries;
	unsigned input_value;
	input_value = 1;
	entries = list_print();

	if ( entries > 0 )
	{
		while ( input_value != 0 )
		{
			if ( num < 0 || entries < (unsigned) num )
			{
				printf( "Select entry for deletion (1...%u). Select 0 for abortion.\n", entries );
				if ( scanf( "%u", &input_value ) == 0 )
					fprintf( stderr, "scanf err\n" );
			}
			else
				input_value = num;

			if ( input_value > entries || input_value == 0 )
				continue;
			else
			{
				FILE * config;
				FILE * cf_tmp;
				char bufline[BUFSIZ];
				unsigned i;

				/* copy saved strings to tmp file */
				config = fopen( CONF_PATH, "r" );
				cf_tmp = fopen( CONF_TMP_FILE, "w" );

				for ( i = 1; i <= ( entries * 2 ); ++i )
					if ( fgets( bufline, BUFSIZ, config ) != NULL )
						if ( i != ( 2 * input_value - 1) && i != ( 2 * input_value ) )
							fprintf( cf_tmp, "%s", bufline );

				fclose( config );
				fclose( cf_tmp );

				/* copy to original file */
				config = fopen( CONF_PATH, "w" );
				cf_tmp = fopen( CONF_TMP_FILE, "r" );

				for ( i = 1; i <= ( (entries - 1) * 2 ); ++i )
					if ( fgets( bufline, BUFSIZ, cf_tmp ) != NULL )
						fprintf( config, "%s", bufline );

				fclose( config );
				fclose( cf_tmp );
				remove( CONF_TMP_FILE );

				printf("\nEntry %u removed", input_value);
				input_value = 0;
			}
		}

	}
	else
		puts("No entries");
}

void
main_loop( void )
{
	FILE * config;
	char bufline[BUFSIZ];
	char notif_command[BUFSIZ];
	double time_difference;
	int ret;
	short last_activated_entry, todo_active;
	time_t cur_time, time_buf;
	unsigned k, lines;
	struct tm tmp_time;

	while (1)
	{
		/* fill struct */
		lines = 0;
		config = fopen( CONF_PATH, "a+" );
		while( feof(config) == 0 )
			if ( fgets( bufline, BUFSIZ, config ) != NULL )
				++lines;

		lines /= 2;
		if ( lines > 0 )
		{
			struct entry * en;
			en = malloc( sizeof( struct entry ) * lines + 1 );
			rewind( config );
			for ( k = 0; k < lines; ++k )
			{
				if ( fgets( bufline, BUFSIZ, config ) != NULL )
					sscanf( bufline, "%d %d.%d.%d %d:%d",
					        &en[k].type, &en[k].et.tm_mday, &en[k].et.tm_mon, &en[k].et.tm_year, &en[k].et.tm_hour, &en[k].et.tm_min );

				if ( fgets( en[k].text, BUFSIZ, config ) != NULL )
					en[k].text[ strlen(en[k].text) - 1 ] = '\0';
			}

			/* struct tm conversion to time_t for difftime */
			time( &cur_time );
			todo_active = 0;
			last_activated_entry = 0;
			for ( k = 0; k < lines; ++k )
			{
				/* mktime uses garbage from somewhere, so we'll use nullified struct tm instance */
				memset( &tmp_time, 0, sizeof( struct tm ) );
				tmp_time.tm_year = en[k].et.tm_year - 1900;
				tmp_time.tm_mon = en[k].et.tm_mon - 1;
				tmp_time.tm_mday = en[k].et.tm_mday;
				tmp_time.tm_hour = en[k].et.tm_hour;
				tmp_time.tm_min = en[k].et.tm_min;
				time_buf = mktime( &tmp_time );
				time_difference = difftime( time_buf, cur_time );
				if ( time_difference <= 0 )
				{
					++todo_active;
					last_activated_entry = k;
				}
			}

			if ( todo_active )
			{
				sprintf( notif_command, "%s \"TODO: %s (%d more entries)\"",
				         NOTIF_COM, en[last_activated_entry].text, --todo_active );

				ret = system( notif_command );
				if ( (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) )
					fprintf( stderr, "system() error\n" );
			}

			free( en );
		}

		/* idle phase */
		fclose( config );
		sleep(REFRESH_RATE);
	}
}

int
main( int argc, char ** argv )
{
	short i;

	set_path();

	/* Reading arguments */
	for ( i = 1; i < argc; ++i )
	{
		if ( argv[i][0] == '-' )
		{
			if ( argv[i][1] == 't' )
				REFRESH_RATE = (unsigned) atoi ( argv[i+1] );

			switch( argv[i][1] )
			{
			case 'L':
				printf( "Total entries: %u\n", list_print() );
				return 0;
			case 'r':
				if ( argc > i+1 && argv[i+1][0] != '-' )
					delete_entry( atoi( argv[i+1] ) );
				else
					delete_entry( -1 );
				return 0;
			case 'a':
				add_task( argc, argv );
				return 0;
			case 'h':
				help_print();
				return 0;
			case 'd':
				main_loop();
				break;
			default:
				return 0;
			}
		}
	}

	/* no valid arguments, print help and exit */
	help_print();
	return 0;
}
