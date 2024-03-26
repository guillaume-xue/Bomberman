CC=gcc
CFLAGS=-Wall -o

all: main client server

main: main.c
	$(CC) $(CFLAGS) main main.c	-lcurses

client: client.c
	$(CC) $(CFLAGS) client client.c

server: server.c
	$(CC) $(CFLAGS) server server.c

clean:
	rm -f main client server