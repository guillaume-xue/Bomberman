#ifndef SERVER_H
#define SERVER_H

#include "config.h"
#include "func.h"

#define BOMB_TIMER 5
#define EXPLOSION_RADIUS 1

void init_mutex();
void add_player(Partie *partie, int client_socket);
void add_partie(int client_socket, int mode_jeu);
int join_or_create(int client_socket, int mode_jeu);
void *handle_client(void *arg);
void init_multicast_socket(Partie *partie);
void send_game_s_info(Partie *partie, int client_socket);
void *game_communication(void *arg);

#endif
