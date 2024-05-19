# Variables de compilation
CC = gcc
CFLAGS = -Wall -g
LIBS = -lcurses -lpthread

# Répertoires des sources
SRC_DIR = src
CLIENT_DIR = $(SRC_DIR)/client
SERVER_DIR = $(SRC_DIR)/server
COMMON_DIR = $(SRC_DIR)/common

# Fichiers sources et objets
CLIENT_SRCS = $(CLIENT_DIR)/client.c $(CLIENT_DIR)/grid.c $(CLIENT_DIR)/main_client.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

SERVER_SRCS = $(SERVER_DIR)/server.c $(COMMON_DIR)/func.c $(SERVER_DIR)/main_server.c $(SERVER_DIR)/game_logic.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

# Cible par défaut
all: bin comp_client comp_server

# Compilation du client
comp_client: $(CLIENT_OBJS) $(COMMON_DIR)/func.o
	$(CC) -o bin/client $(CLIENT_OBJS) $(COMMON_DIR)/func.o $(LIBS)

# Compilation du serveur
comp_server: $(SERVER_OBJS)
	$(CC) -o bin/server $(SERVER_OBJS) $(LIBS)

# Règle de compilation pour les fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

client :
	./bin/client

server :
	./bin/server

# Nettoyage des fichiers objets et exécutables
clean:
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS) $(COMMON_DIR)/func.o bin/client bin/server bin/client

# Nettoyage complet
clean: clean

# Création du répertoire bin si nécessaire
bin:
	mkdir -p bin

.PHONY: all client server clean clean
