CC=gcc
CFLAGS=-Wall
LIBS=-lcurses -lpthread

all: client server 

client: client.c client.h grid.c grid.h config.h
	$(CC) $(CFLAGS) -o client grid.c client.c $(LIBS)

server: server.c server.h 
	$(CC) $(CFLAGS) -o server server.c $(LIBS)

main: main.c
	$(CC) $(CFLAGS) -o main main.c -lcurses

clean:
	rm -f client server grid main
