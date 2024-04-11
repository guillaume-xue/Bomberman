#ifndef SERVER_H
#define SERVER_H

#include "config.h"

char *get_color(int x);

void init_mutex();
void init_parties();
void add_player(Partie *partie);
void add_partie(int client_socket, int mode_jeu, int index_partie);
int join_or_create(int client_socket, int mode_jeu);
void *handle_client(void *arg);
int recv_mode(int client_socket);
void x_client_left(int x, int client_socket);
void init_multicast_socket(Partie *partie);

void send_game_s_info(int client_socket, int id_player, int id_partie );

#endif
