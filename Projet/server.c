#include "server.h"

Partie parties[MAX_PARTIES];

int nb_partie = 0;

pthread_mutex_t mutex_partie = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_partie = PTHREAD_COND_INITIALIZER;

void add_partie(int client_socket, int mode_jeu) {
  int i = nb_partie;

  parties[i].mode_jeu = mode_jeu;
  parties[i].clients_socket_tcp[parties[i].nb_joueurs] = client_socket;
  parties[i].clients_playing[parties[i].nb_joueurs] = parties[i].nb_joueurs;
  parties[i].partie_id = i;

  init_multicast_socket(&parties[i]);
  add_player(&parties[i], client_socket);
}

void add_player(Partie *partie, int client_socket) {
  partie->clients_socket_tcp[partie->nb_joueurs] = client_socket;
  partie->clients_playing[partie->nb_joueurs] = partie->nb_joueurs;
  partie->nb_joueurs++;

  send_game_s_info(partie, client_socket);
}

int join_or_create(int client_socket, int mode_jeu) {
  pthread_mutex_lock(&mutex_partie);

  for (int i = 0; i < nb_partie; i++) {
    if (parties[i].mode_jeu != -1) {
      if (parties[i].mode_jeu == mode_jeu &&
          parties[i].nb_joueurs < MAX_CLIENTS) {
        add_player(&parties[i], client_socket);
        pthread_mutex_unlock(&mutex_partie);
        return i;
      }
    }
  }

  if (nb_partie < MAX_PARTIES) {
    add_partie(client_socket, mode_jeu);
    nb_partie++;
  }

  pthread_mutex_unlock(&mutex_partie);
  return nb_partie - 1; // retourne l'index de la partie tout juste créer
}

// Fonction pour envoyer les informations de la partie au joueur
void send_game_s_info(Partie *partie, int client_socket) {
  ServerMessage server_message;
  server_message.code_req = (parties[partie->partie_id].mode_jeu == 1) ? 9 : 10;

  server_message.id = parties[partie->partie_id].nb_joueurs;

  server_message.eq = parties[partie->partie_id].mode_jeu == 1
                          ? 1
                          : (server_message.id % 2) + 1;

  server_message.port_m_diff =
      parties[partie->partie_id].multicast_addr.sin6_port;

  inet_ntop(AF_INET6, &(parties[partie->partie_id].multicast_addr.sin6_addr),
            server_message.adr_m_diff, INET6_ADDRSTRLEN);

  if (send(client_socket, &server_message, sizeof(ServerMessage), 0) < 0) {
    perror("L'envoi des informations de la partie a échoué");
    exit(EXIT_FAILURE);
  }
}

void init_multicast_socket(Partie *partie) {
  // Création de la socket
  partie->send_sock = socket(AF_INET6, SOCK_DGRAM, 0);
  if (partie->send_sock < 0) {
    perror("socket for sending failed");
    exit(EXIT_FAILURE);
  }

  /* Initialisation de l'adresse d'abonnement */
  memset(&partie->multicast_addr, 0, sizeof(partie->multicast_addr));

  char multicast_group[INET6_ADDRSTRLEN];
  snprintf(multicast_group, sizeof(multicast_group), "ff02::1:2:3:%x",
           partie->partie_id + 1);

  /* Initialisation de l'adresse d'abonnement */
  partie->multicast_addr.sin6_family = AF_INET6;
  inet_pton(AF_INET6, multicast_group, &partie->multicast_addr.sin6_addr);
  partie->multicast_addr.sin6_port = htons(MULTICAST_PORT + partie->partie_id);

  int ifindex = if_nametoindex("en0");
  if (ifindex == 0) {
    perror("if_nametoindex");
    close(partie->send_sock);
    exit(EXIT_FAILURE);
  }

  partie->multicast_addr.sin6_scope_id = ifindex;
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;

    EnteteMessage received_message;
    memset(&received_message, 0, sizeof(EnteteMessage));

    // Réception du message depuis le client
    if (recv(client_socket, &received_message, sizeof(EnteteMessage), 0) < 0) {
        perror("La réception du message a échoué");
        exit(EXIT_FAILURE);
    }

    int index_partie = join_or_create(
    client_socket, received_message.CODEREQ); // index_partie est l'index de la partie où le client a rejoint

    GameMessage client_ready;
    memset(&client_ready, 0, sizeof(GameMessage));

    if (recv(client_socket, &client_ready, sizeof(GameMessage), 0) < 0) {
        perror("La réception du message a échoué");
        exit(EXIT_FAILURE);
    }

    char buf[SIZE_MSG];
    memset(buf, 0, sizeof(buf));

    if (client_ready.EQ == -1) {
        snprintf(buf, SIZE_MSG,
        "\nLe client devient : %sJoueur n.%d%s et rejoint la partie n.%d "
        "en mode : SOLO (1v3).",
        id_to_color(client_ready.ID), client_ready.ID, "\33[0m",
        index_partie);
    } else {
        snprintf(buf, SIZE_MSG,
        "\nLe client devient : %sJoueur n.%d%s dans l'équipe %d et "
        "rejoint la partie n.%d en mode : MULTIJOUEUR (2v2).",
        id_to_color(client_ready.ID), client_ready.ID, "\33[0m",
        client_ready.EQ, index_partie);
    }

    printf("%s\n", buf);

    pthread_mutex_lock(&mutex_partie);
    printf("Il reste %d places dans la partie n.%d %s\n",
        MAX_CLIENTS - parties[index_partie].nb_joueurs, index_partie,
        (MAX_CLIENTS - parties[index_partie].nb_joueurs == 0)
        ? "\n\033[31m\nNous sommes au complet, la partie peut commencer "
        "!!\033[0m\n"
        : "\n");

    if (MAX_CLIENTS - parties[index_partie].nb_joueurs == 0) {
        pthread_mutex_unlock(&mutex_partie);
        printf("\033[90mLa partie n.%d va commencer dans 3 secondes.\033[0m\n",
        index_partie);
    for (int i = 0; i < 3; i++) {
        printf("\033[90m%d\033[0m \n", 3 - i);
        sleep(1);
    }
    puts("\033[90mGOOOOOOO\nLa partie commence !!\033[0m\n\n\n\n");

    // Initialisation de la grille
    GridData g;
    memset(&g, 0, sizeof(GridData) - 1);

    // Initialisation des joueurs
    player *p = malloc(4 * sizeof(player *));
    if (p == NULL) {
        perror("Memory allocation error for 'p'");
        exit(EXIT_FAILURE);
    }
    setup_grid(&g, 20, 51, p);

    // Envoie de la grille initiale
    if (sendto(parties[index_partie].send_sock, &g, sizeof(GridData) - 1, 0,
            (struct sockaddr *)&parties[index_partie].multicast_addr,
            sizeof(parties[index_partie].multicast_addr)) < 0) {
        perror("L'envoi de la grille a échoué");
        exit(EXIT_FAILURE);
    }

    printf("La grille a été envoyée\n");

    // Envoie des cases de la grille
    if (sendto(parties[index_partie].send_sock, g.cases, g.hauteur * g.largeur * sizeof(ContenuCase) - 1, 0,
               (struct sockaddr *)&parties[index_partie].multicast_addr,
               sizeof(parties[index_partie].multicast_addr)) < 0) {
        perror("L'envoi de la grille a échoué");
        exit(EXIT_FAILURE);
    }

    //pthread_t thread_grid;
    //int *x = malloc(sizeof(int));
    //*x = nb_partie;
    //pthread_create(&thread_grid, NULL, game_comm, x);
    } else {
    pthread_mutex_unlock(&mutex_partie);
    }

    free(arg);
    return NULL;
}

int check_maj(GridData *grid_handler, Partie partie) {
  // On vérifie si l'action du joueur est legit
  return 0;
}

void *game_comm(void *arg) {
  int partie_id = *(int *)arg;

  GameMessage gameMessage;
  while (1) {

    memset(&gameMessage, 0, sizeof(GameMessage));
    if (recv(parties[partie_id].send_sock, &gameMessage, sizeof(GameMessage),
             0) < 0) {
      perror("La réception de la grille a échoué");
      exit(EXIT_FAILURE);
    }

    GridData grid;
    memset(&grid, 0, sizeof(GridData));

    player *p = malloc(4 * sizeof(player *));

    setup_grid(&grid, 10, 10, p);

    start_game(grid, p, gameMessage);

    if (sendto(parties[partie_id].send_sock, &gameMessage, sizeof(GameMessage),
               0, (struct sockaddr *)&parties[partie_id].multicast_addr,
               sizeof(parties[partie_id].multicast_addr)) < 0) {
      perror("L'envoi de la grille a échoué");
      exit(EXIT_FAILURE);
    }

    sleep(INTERVALLE_ENVOI);
  }

  return NULL;
}

void *handle_partie(void *arg) {
  int client_socket = *(int *)arg;

  int *x = malloc(sizeof(int));
  if (!x) {
    perror("L'allocation de la mémoire a échoué");
    exit(EXIT_FAILURE);
  }

  *x = client_socket;
  pthread_t thread_client;

  void *ret = NULL;

  pthread_create(&thread_client, NULL, handle_client, x);
  pthread_join(thread_client, &ret);

  // if (*(int) ret == -1) {
  //   printf("Le client a quitté la partie\n");
  // }

  return NULL;
}

int main() {
  int server_socket;
  struct sockaddr_in6 server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);

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

  printf("Serveur démarré, en attente de connexions...\n\n");

  while (1) {
    int client_socket_tcp =
        accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

    if (client_socket_tcp >= 0) {
      printf("Ohhh, un client a rejoint le serveur\n");

      int *x = malloc(sizeof(int));
      if (!x) {
        perror("L'allocation de la mémoire a échoué");
        exit(EXIT_FAILURE);
      }

      *x = client_socket_tcp;
      pthread_t thread_client;
      pthread_create(&thread_client, NULL, handle_client, x);
    }
  }

  close(server_socket);

  return 0;
}
