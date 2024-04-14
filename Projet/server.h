#ifndef SERVER_H
#define SERVER_H

#include "config.h"
#include "client.h"
#include "func.h"

#define MAX_CLIENTS 4
#define MAX_PARTIES 4

typedef struct {
  int nb_joueurs;
  int partie_id;
  int mode_jeu;

  pthread_t thread_partie;  // thread pour gérer la partie, afin de pas bloquer le serveur

  int send_sock;  // sock pour envoyer des messages multicast aux clients
  struct sockaddr_in6 multicast_addr; // adresse multicast

  struct sockaddr_in6 partie_addr;  // adresse de la partie

  int clients_socket_tcp[MAX_CLIENTS];  // sockets des clients, pour communiquer avec eux
  int clients_playing[MAX_CLIENTS];  // id des clients, afin de savoir qui est encore connecté dans la partie
} Partie;

typedef struct {
  int code_req;
  short id;
  short eq;
  uint16_t port_udp;
  uint16_t port_m_diff;
  char adr_m_diff[INET6_ADDRSTRLEN];
} ServerMessage;

char *get_color(int x);

void init_mutex();
void init_parties();
void add_player(Partie *partie, int client_socket);
void add_partie(int client_socket, int mode_jeu);
int join_or_create(int client_socket, int mode_jeu);
void *handle_client(void *arg);
void init_multicast_socket(Partie *partie);
void send_game_s_info(Partie *partie, int client_socket);

#endif
