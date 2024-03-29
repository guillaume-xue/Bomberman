CC=gcc
CFLAGS=-Wall -o

all: grid client server

grid: grid.c
	$(CC) $(CFLAGS) grid grid.c	-lcurses

client: client.c
	$(CC) $(CFLAGS) client client.c

server: server.c
	$(CC) $(CFLAGS) server server.c

clean:
	rm -f client server grid