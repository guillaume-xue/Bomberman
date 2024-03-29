#define MAX_CLIENTS 4
#define MAX_PARTIES 4

#include "config.h"

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
} Partie;

int ask_game_mode(int client_socket);
enum colors get_color(int x);
void add_player(int p_id, int t_num, int sock_c, int index);
void add_partie(int client_socket, int mode_jeu);
void join_or_create(int client_socket, int mode_jeu);
void handle_client(int client_socket);
int ask_game_mode(int client_socket);