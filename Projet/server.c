#include "server.h"

#define MAX_PARTIES 4

Partie parties[MAX_PARTIES];

int nb_partie = 0;
GridData* grille;

pthread_mutex_t mutex_partie = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_partie = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_joueur[100];
pthread_cond_t cond_joueur[100];

void init_mutex() {
  for (int i = 0; i < 100; i++) {
    pthread_mutex_init(&mutex_joueur[i], NULL);
    pthread_cond_init(&cond_joueur[i], NULL);
  }
}


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
      if (parties[i].mode_jeu == mode_jeu && parties[i].nb_joueurs < 4) {
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
  snprintf(multicast_group, sizeof(multicast_group), "ff02::1:2:3:%x", partie->partie_id + 1);
  
/* Initialisation de l'adresse d'abonnement */
  partie->multicast_addr.sin6_family = AF_INET6;
  inet_pton(AF_INET6, multicast_group, &partie->multicast_addr.sin6_addr);
  partie->multicast_addr.sin6_port = htons(MULTICAST_PORT + partie->partie_id);
  
  int ifindex = if_nametoindex("eth0");
  if (ifindex == 0) {
    perror("if_nametoindex");
    close(partie->send_sock);
    exit(EXIT_FAILURE);
  }

  partie->multicast_addr.sin6_scope_id = ifindex;

  printf("Multicast Address: %s Port: %d Scope ID: %d\n", multicast_group, ntohs(partie->multicast_addr.sin6_port), partie->multicast_addr.sin6_scope_id);
}


GridData* init_grid_data(uint8_t longueur, uint8_t largeur) {
    GridData* grid = (GridData*)malloc(sizeof(GridData));
    if (grid == NULL) {
        perror("Erreur lors de l'allocation mémoire pour GridData");
        exit(EXIT_FAILURE);
    }

    grid->longueur = longueur;
    grid->largeur = largeur;

    grid->cases = (uint8_t*)malloc(longueur * largeur * sizeof(uint8_t));
    if (grid->cases == NULL) {
        perror("Erreur lors de l'allocation mémoire pour le tableau de cases");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < longueur * largeur; i++) {
        grid->cases[i] = 0;
    }

    return grid;
}


void envoyer_grille(Partie *partie) {
    // Taille totale de la grille à envoyer (entête + longueur + largeur + cases)
    size_t taille_grille = sizeof(uint8_t) + sizeof(uint8_t) +
                           grille->longueur * grille->largeur * sizeof(uint8_t);

    uint8_t *buffer = (uint8_t *)malloc(taille_grille);
    if (buffer == NULL) {
        perror("Erreur d'allocation mémoire pour le buffer de la grille");
        exit(EXIT_FAILURE);
    }

    memcpy(buffer, &(grille->longueur), sizeof(uint8_t));
    buffer += sizeof(uint8_t);

    memcpy(buffer, &(grille->largeur), sizeof(uint8_t));
    buffer += sizeof(uint8_t);

    memcpy(buffer, grille->cases, grille->longueur * grille->largeur * sizeof(uint8_t));

    // Réinitialiser le pointeur du buffer
    buffer -= taille_grille - sizeof(EnteteMessage);

    // Envoyer le buffer contenant la grille en multicast
    ssize_t bytes_sent;
    struct sockaddr_in6 gradr;
    memset(&gradr, 0, sizeof(gradr));
    gradr.sin6_family = AF_INET6;
    gradr.sin6_addr = partie->multicast_addr.sin6_addr;
    gradr.sin6_port = htons(partie->multicast_addr.sin6_port);

    gradr.sin6_scope_id = partie->multicast_addr.sin6_scope_id;
    bytes_sent = sendto(partie->send_sock, buffer, taille_grille, 0, (struct sockaddr *)&gradr, sizeof(struct sockaddr_in6));
    if (bytes_sent == -1) {
        perror("Erreur lors de l'envoi de la grille en multicast");
    }

 }

void* signalement_debut_partie(void *arg) {
    Partie* partie = (Partie *)arg;

    char message[SIZE_MSG];
    snprintf(message, SIZE_MSG, "La partie a commencé !");
    ssize_t size;

    struct sockaddr_in6 gradr;
    memset(&gradr, 0, sizeof(gradr));
    gradr.sin6_family = AF_INET6;
    gradr.sin6_addr = partie->multicast_addr.sin6_addr;
    gradr.sin6_port = htons(partie->multicast_addr.sin6_port);

    gradr.sin6_scope_id = partie->multicast_addr.sin6_scope_id;

    if ((size = sendto(partie->send_sock, message, strlen(message), 0, (struct sockaddr *)&gradr, sizeof(struct sockaddr_in6))) < 0){
        perror("erreur send");
    }

    printf("send : %ld\n", size);

    grille = init_grid_data(10,10);
    while(1){
         envoyer_grille(partie);
    }
    return NULL;
}


void *handle_client(void *arg) {
  int client_socket = *(int *)arg;

  GameMessage received_message;
  memset(&received_message, 0, sizeof(GameMessage));

  // Réception du message depuis le client
  if (recv(client_socket, &received_message, sizeof(GameMessage), 0) < 0) {
    perror("La réception du message a échoué");
    exit(EXIT_FAILURE);
  }

  int index_partie = join_or_create(
      client_socket,
      received_message.CODEREQ); // index_partie est l'index de la partie où le
                                 // client a rejoint

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
  printf("Il reste %d places dans la partie n.%d %s\n",
         4 - parties[index_partie].nb_joueurs, index_partie,
         (4 - parties[index_partie].nb_joueurs == 0)
             ? "\n\033[31m\nNous sommes au complet, la partie peut commencer "
               "!!\033[0m\n"
             : "\n");

  if (4 - parties[index_partie].nb_joueurs == 0) {
    printf("\033[90mLa partie n.%d va commencer dans 5 secondes.\033[0m\n",
           index_partie);
    for (int i = 0; i < 5; i++) {
      printf("\033[90m%d\033[0m \n", 5 - i);
      sleep(1);
    }
    puts("\033[90mGOOOOOOO\nLa partie commence !!\033[0m\n\n\n\n");

    pthread_t tid;
    if (pthread_create(&tid, NULL, signalement_debut_partie, (void*)&parties[index_partie]) != 0) {
        perror("Erreur lors de la création du thread");
        exit(EXIT_FAILURE);
    }
    //pthread_join(tid, NULL);
  }

  return NULL;
}


void *handle_partie(void *arg) {
  int client_socket = *(int *)arg;

  int *x = malloc(sizeof(int));
  if (!x) {
    perror("L'allocation de la mémoire a échoué");
    exit(EXIT_FAILURE);
  } else {
    *x = client_socket;
    pthread_t thread_client;
    pthread_create(&thread_client, NULL, handle_client, x);
  }

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
      pthread_create(&thread_client, NULL, handle_partie, x);
    }
  }

  close(server_socket);

  return 0;
}
