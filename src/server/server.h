#ifndef SERVER_H
#define SERVER_H

#include <poll.h>
#include "../common/config.h"
#include "../common/func.h"
#include "game_logic.h"

void init_mutex();
void add_player(Partie *partie, int client_socket);
void add_partie(int client_socket, int mode_jeu);
int join_or_create(int client_socket, int mode_jeu);
void init_multicast_socket(Partie *partie);
void send_game_s_info(Partie *partie, int client_socket);
void *game_communication_freq(void *arg);
void handle_client_poll(int client_socket);

#endif
