#define _GNU_SOURCE
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NOTIF_COM "notify-send"
#define CONF_FILE "nachalnick.conf"
#define FORMAT_DT "dd.mm.yyyy "
#define FORMAT_TM "HH:MM"
#define BUFSIZE 512
#define REFRESH_RATE 600
char CONF_PATH[BUFSIZE];

struct entry
{
	int type;
	struct tm et;
	char text[BUFSIZE];
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
	char in_date[strlen( FORMAT_DT )];
	char in_time[strlen( FORMAT_TM )];
	char in_text[BUFSIZE];
	short i;
	for ( i = 1; i < argc; ++i )
	{
		if ( argv[i][0] == '-' && argv[i][1] == 'a' )
		{
			in_type = atoi( argv[i+1] );
			strncpy( in_date, argv[i+2], strlen( FORMAT_DT ) );
			strncpy( in_time, argv[i+3], strlen( FORMAT_TM ) );
			strncpy( in_text, argv[i+4], BUFSIZE );
		}
	}
	FILE * config;
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
	puts("\twhere TYPE 0 means constant reminding");
	puts("\n  List entries:");
	puts("\t-L");
	puts("\n  Remove entry #NUMBER:");
	puts("\t-r NUMBER");
}

int
main( int argc, char ** argv )
{
	set_path();

	/* Reading arguments */
	short i;
	for ( i = 1; i < argc; ++i )
	{
		if ( argv[i][0] == '-' )
		{
			if ( argv[i][1] == 'a' )
			{
				add_task( argc, argv );
				return 0;
			}
			if ( argv[i][1] == 'h' )
			{
				help_print();
				return 0;
			}
			if ( argv[i][1] == 'd' )
			{
				FILE * config;
				char * bufline;
				size_t linelen = 0;
				ssize_t gline;
				unsigned k;
				unsigned lines;

				/* daemon loop, sort of */
				while (1)
				{
					/* fill structs */
					lines = 0;
					config = fopen( CONF_PATH, "r" );
					while( feof(config) == 0 )
					{
						gline = getline( &bufline, &linelen, config);
						if ( gline > 0 )
							++lines;
					}

					lines /= 2;
					printf("entries: %u\n", lines);
					rewind( config );

					struct entry en[lines];
					for ( k = 0; k < lines; ++k )
					{
						if ( fgets( bufline, BUFSIZE, config ) == NULL )
							fprintf( stderr, "string err\n" );

						sscanf( bufline, "%d %d.%d.%d %d:%d",
								&en[k].type, &en[k].et.tm_mday, &en[k].et.tm_mon, &en[k].et.tm_year, &en[k].et.tm_hour, &en[k].et.tm_min );

						if ( fgets( en[k].text, BUFSIZE, config ) == NULL )
							fprintf( stderr, "string err\n" );

						printf("%d: %d %s", k + 1, en[k].type, en[k].text);
					}
					fclose( config );


					sleep(REFRESH_RATE);
				}
			}
		}
	}

	/* no valid arguments, print help and exit */
	help_print();
	return 0;
}
