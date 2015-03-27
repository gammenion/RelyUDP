CC = gcc
CFLAGS = -Wall -g -D_DEBUG
LINK = -lpthread

OBJ= conn.o net.o list.o

TEST = server client

#all: ${OBJ} conn
all: ${OBJ} ${TEST}

test: ${TEST}

server: server.c ${OBJ}
	${CC} ${CFLAGS} ${LINK} -o $@ $< ${OBJ}

client: client.c ${OBJ}
	${CC} ${CFLAGS} ${LINK} -o $@ $< ${OBJ}

conn: conn.c
	${CC} ${CFLAGS} ${LINK} -D_DEBUG_$@_ -o $@ $< net.o list.o

list: list.c
	${CC} ${CFLAGS} ${LINK} -D_DEBUG_$@_ -o $@ $<

clean:
	rm -f ${OBJ} ${TEST}
