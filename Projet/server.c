#include "server.h"

#define MAX_PARTIES 4

// variables globales
Partie parties[MAX_PARTIES];
int index_partie = 0;

pthread_mutex_t mutex_partie = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_partie = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_joueur[100];
pthread_cond_t cond_joueur[100];

int *tab_socket[100]; // tableau de pointeurs de socket, qui représente le tube
                      // de communication entre le serveur et les clients
// variables globales

void init_mutex() {
  for (int i = 0; i < 100; i++) {
    pthread_mutex_init(&mutex_joueur[i], NULL);
    pthread_cond_init(&cond_joueur[i], NULL);
  }
}

void *handle_partie(void *arg) {
  Partie *partie = (Partie *)arg;
  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));

  while (1) {
    ssize_t received = recv(partie->listen_sock, buf, SIZE_MSG, 0);
    if (received < 0) {
      perror("La réception de l'adresse et du port UDP a échoué");
      exit(EXIT_FAILURE);
    }

    if (strcmp(buf, "quit") == 0) {
      partie->nb_joueurs--;
      if (partie->nb_joueurs == 0) {
        close(partie->send_sock);
        close(partie->listen_sock);
        pthread_cancel(partie->thread_partie);
      }
    }

    printf("Le message reçu est : '%s'\n", buf);

    memset(buf, 0, sizeof(buf));
    if (partie->nb_joueurs == 4) {
      sprintf(buf, "Tous les joueurs sont prêts, la partie peut commencer");
    } else {
      sprintf(buf, "Il reste %d joueurs avant que la partie commence",
              4 - partie->nb_joueurs);
    }

    if (sendto(partie->send_sock, buf, strlen(buf), 0,
               (struct sockaddr *)&partie->multicast_addr,
               sizeof(partie->multicast_addr)) < 0) {
      perror("L'envoi du message de prêt a échoué");
      exit(EXIT_FAILURE);
    }
  }
}

void add_partie(int client_socket, int mode_jeu) {
  parties[index_partie].nb_joueurs = 0;
  parties[index_partie].partie_id = index_partie;
  parties[index_partie].mode_jeu = mode_jeu;
  parties[index_partie].nb_joueurs = 0;
  parties[index_partie].listen_sock = client_socket;
  parties->partie_addr.sin6_family = AF_INET6;
  parties->partie_addr.sin6_port = htons(TCP_PORT + parties->partie_id);
  inet_pton(AF_INET6, "::1", &parties->partie_addr.sin6_addr);
  init_multicast_socket(&parties[index_partie]);

  pthread_create(&parties[index_partie].thread_partie, NULL, handle_partie,
                 (void *)&parties[index_partie]);
}

void init_multicast_socket(Partie *partie) {
  partie->send_sock = socket(AF_INET6, SOCK_DGRAM, 0);
  if (partie->send_sock < 0) {
    perror("socket for sending failed");
    exit(EXIT_FAILURE);
  }

  memset(&partie->multicast_addr, 0, sizeof(partie->multicast_addr));
  partie->multicast_addr.sin6_family = AF_INET6;
  partie->multicast_addr.sin6_port = htons(MULTICAST_PORT + partie->partie_id);

  char multicast_group[40];
  snprintf(multicast_group, sizeof(multicast_group), "ff02::1:2:3:%x",
           partie->partie_id + 1);
  inet_pton(AF_INET6, multicast_group, &partie->multicast_addr.sin6_addr);

  int ifindex = if_nametoindex("en0");
  if (ifindex == 0) {
    perror("if_nametoindex");
    close(partie->send_sock);
    exit(EXIT_FAILURE);
  }

  if (setsockopt(partie->send_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex,
                 sizeof(ifindex)) < 0) {
    perror("setsockopt");
    close(partie->send_sock);
    exit(EXIT_FAILURE);
  }
}

void join_or_create(int client_socket, int mode_jeu) {
  for (int i = index_partie; i >= 0; i--) {
    if (parties[i].mode_jeu == mode_jeu && parties[i].nb_joueurs < 4) {
      int x = ++parties[i].nb_joueurs;
      send_suscribe_info(x - 1);
      // if (x == 4)
      //   send_multicast_message();

      return;
    }
  }

  pthread_mutex_lock(&mutex_partie);
  index_partie++;
  add_partie(client_socket, mode_jeu);
  pthread_mutex_lock(&mutex_partie);
}

int send_suscribe_info(int index_player) {
  int player_id = (index_player * 4) + parties[index_partie].nb_joueurs;
  int team_number;
  if (parties[index_partie].mode_jeu == 1) {
    team_number = 1;
  } else {
    team_number = (player_id % 2) + 1;
  }
  int multicast_port = parties[index_partie].multicast_addr.sin6_port;
  int partie_port = parties[index_partie].partie_addr.sin6_port;
  char multicast_addr[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &(parties[index_partie].multicast_addr.sin6_addr),
            multicast_addr, INET6_ADDRSTRLEN);
  char *color = get_color(player_id);

  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, SIZE_MSG, "%d,%d,%d,%d,%s,%s", player_id, team_number,
           multicast_port, partie_port, multicast_addr, color);
  if (send(*(tab_socket[index_partie]), buf, strlen(buf), 0) < 0) {
    perror("L'envoi des informations de souscription a échoué");
    exit(EXIT_FAILURE);
  }

  return 0;
}

// servira peut etre plus tard
// void x_client_left(int x, int client_socket) {
//   int res = 4 - x;
//   char buf[SIZE_MSG];
//   memset(buf, 0, sizeof(buf));
//   snprintf(buf, SIZE_MSG, "%d", res);

//   if (send(client_socket, buf, strlen(buf), 0) < 0) {
//     perror("L'envoi du nombre de joueurs restants a échoué");
//     exit(EXIT_FAILURE);
//   }
// }

void *handle_client(void *arg) {
  int client_socket = *(int *)arg;

  int mode_jeu = recv_mode(client_socket);

  join_or_create(client_socket, mode_jeu);

  return NULL;
}

char *get_color(int x) {
  switch (x % 4) {
  case 0:
    return "\033[0;32m";
  case 1:
    return "\033[0;33m";
  case 2:
    return "\033[0;35m";
  case 3:
    return "\033[0;36m";
  }

  return "\033[0m";
}

int recv_mode(int client_socket) {
  int player_choice;

  puts("Attente du choix du joueur pour le mode de jeu");

  if (recv(client_socket, &player_choice, sizeof(player_choice), 0) < 0) {
    perror("La réception de la demande de mode de jeu a échoué");
    exit(EXIT_FAILURE);
  }

  printf("Le joueur a choisi le mode de jeu : %s\n",
         (player_choice == 1 ? "1v3" : "2v2"));

  return player_choice;
}

int main() {
  int server_socket;
  struct sockaddr_in6 server_addr, client_addr;

  server_socket = socket(AF_INET6, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("La création de la socket a échoué");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(TCP_PORT);
  inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("La liaison a échoué");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket, 5) < 0) {
    perror("Écoute des connexions a échoué");
    exit(EXIT_FAILURE);
  }

  printf("Serveur démarré, en attente de connexions...\n");

  socklen_t addr_len = sizeof(client_addr);
  while (1) {
    tab_socket[index_partie] = malloc(sizeof(int));
    *(tab_socket[index_partie]) =
        accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

    if (*(tab_socket[index_partie]) >= 0) {
      pthread_t thread_client;
      pthread_create(&thread_client, NULL, handle_client,
                     tab_socket[index_partie]);
    }

    // handle_client(tab_socket);
    // close(tab_socket);
  }

  close(server_socket);

  return 0;
}
