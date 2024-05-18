#ifndef CLIENT_H
#define CLIENT_H

#include "../common/func.h"
#include "../common/config.h"

void connexion_to_tcp_server();
void choose_game_mode();
void *handle_udp(void *arg);
char *id_to_color(int id);
void suscribe_multicast();
void receive_gmsg(int client_socket);
void print_udp_subscription(int sockfd);
void* attente_fin(void *args);
void im_ready();
void launch_game();

#endif