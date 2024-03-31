#ifndef SERVER_H
#define SERVER_H

#include "config.h"
#include "grid.h"
#include "client.h"

#define MAX_CLIENTS 4
#define MAX_PARTIES 4
#define MULTICAST_PORT 10000

typedef struct {
  int player_id;
  int team_number;
  int socket_client;
  enum colors color;
} Joueur;

typedef struct {
  Joueur joueurs[MAX_CLIENTS];
  int nb_joueurs;
  int partie_id;
  int mode_jeu;
  char adresse_multicast[INET6_ADDRSTRLEN];
  int port_udp;
  int port_multicast;
} Partie;

typedef struct {
  int code_req;
  short id;
  short eq;
  uint16_t port_udp;
  uint16_t port_m_diff;
  char adr_m_diff[INET6_ADDRSTRLEN];
} ServerMessage;


enum colors get_color(int x);
void send_game_info(int client_socket, Partie* game);
void add_player(int p_id, int t_num, int sock_c, int index);
void add_partie(int client_socket, int mode_jeu);
void join_or_create(int client_socket, int mode_jeu);
void handle_client(int client_socket);
int ask_game_mode(int client_socket);

#endif