CC=gcc
CFLAGS=-Wall -o
LIBS=-lcurses -lpthread

all: client server 

client: client.c client.h grid.c grid.h config.h
	$(CC) $(CFLAGS) client grid.c client.c $(LIBS)

server: server.c server.h 
	$(CC) $(CFLAGS) server server.c $(LIBS)

grid: grid.c
	$(CC) $(CFLAGS) grid grid.c -lcurses

clean:
	rm -f client server grid
