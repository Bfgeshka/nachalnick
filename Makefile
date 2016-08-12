CC = cc
CFLAGS = -O2 -Wall -Wextra --std=c89 -g

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
