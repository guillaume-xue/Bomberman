#ifndef CLIENT_H
#define CLIENT_H

#include "grid.h"
#include "server.h"

typedef struct {
  int CODEREQ;  // 5: 1v3, 6: 2v2
  short ID; // ID du joueur
  int EQ;   // numéro de l'équipe du joueur si CODEREQ = 6
  int NUM;  // numéro du message modulo 2^13
  int ACTION; // 0: ↑, 1: →, 2: ↓, 3: ←, 4: ↗, 5: ↘, 6: bomb, 7: undo request
} GameMessage;

void send_game_request(int client_socket, int CODEREQ);

void receive_message(int client_socket, void *buffer, size_t buffer_size);

int join_server();

#endif