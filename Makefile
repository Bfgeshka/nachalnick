CC = cc
CFLAGS = -Wall -Wextra -pedantic --std=c89 -pipe -O3 -march=native -s
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
