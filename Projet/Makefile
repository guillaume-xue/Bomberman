CC=gcc
CFLAGS=-Wall
LIBS=-lcurses -lpthread

all: client server 

client: client.c client.h grid.c grid.h config.h
	$(CC) $(CFLAGS) -o client client.c grid.c $(LIBS)

server: server.c server.h 
	$(CC) $(CFLAGS) -o server server.c $(LIBS)

clean:
	rm -f client server 
