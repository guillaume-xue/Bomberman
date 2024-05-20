#ifndef CLIENT_H
#define CLIENT_H

#include "../common/func.h"
#include "../common/config.h"

void connexion_to_tcp_server();
void choose_game_mode();
void suscribe_multicast();
void im_ready();
void launch_game();

#endif