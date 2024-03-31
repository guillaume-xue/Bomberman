#define MAX_CLIENTS 4
#define MAX_PARTIES 4

#include "data.h"

typedef struct {
  int nb_joueurs;
  int partie_id;
  int mode_jeu;

  pthread_t thread_partie;  // thread pour gérer la partie, afin de pas bloquer le serveur

  int send_sock;  // sock pour envoyer des messages multicast aux clients
  struct sockaddr_in6 multicast_addr; // adresse multicast

  int listen_sock;  // sock sur lequel on écoute les connexions des clients
  struct sockaddr_in6 partie_addr;  // adresse de la partie
} Partie;

char *get_color(int x);

void add_partie(int client_socket, int mode_jeu);
void join_or_create(int client_socket, int mode_jeu);
void *handle_client(void *arg);
int recv_mode(int client_socket);
void x_client_left(int x, int client_socket);
void init_multicast_socket(Partie *partie);
int send_suscribe_info(int index_player);
