#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include "func.h"

// WARNING : Faites attention au passage de 8191 à 0.
typedef struct {
  int CODEREQ;  // 5: 1v3, 6: 2v2
  short ID; // ID du joueur
  int EQ;   // numéro de l'équipe du joueur si CODEREQ = 6
  int NUM;  // numéro du message modulo 2^13
  int ACTION; // 0: ↑, 1: →, 2: ↓, 3: ←, 4: ↗, 5: ↘, 6: bomb, 7: undo request
} GameMessage;

void send_game_request(int client_socket, int CODEREQ);
void connexion_to_tcp_server();
void choose_game_mode();
void *handle_udp(void *arg);
char *id_to_color(int id);
void suscribe_multicast();
void receive_gmsg(int client_socket);

#endif