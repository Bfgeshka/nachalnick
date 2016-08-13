CC = cc
CFLAGS = -O3 -Wall -Wextra --std=c89 -pipe
LDFLAGS = -Wl,-O1

NAME = nachalnick
SRC = ${NAME}.c
PREFIX = /usr/local

${NAME}:
	${CC} ${SRC} ${CFLAGS} ${LDFLAGS} -o ${NAME}

clean:
	rm -f ${NAME}

install:
	cp -i ${NAME} ${PREFIX}/bin

uninstall:
	rm -i ${PREFIX}/bin/${NAME}
