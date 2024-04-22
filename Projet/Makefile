CC = gcc
CFLAGS = -Wall
LIBS = -lcurses -lpthread

CLIENT_OBJS = client.o grid_client.o func.o  # Ajoute func.o si nécessaire
SERVER_OBJS = server.o grid_server.o func.o  # Ajoute func.o si nécessaire

all: client server

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o client server
