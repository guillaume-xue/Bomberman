#include "server.h"

int nb_partie = 0;

Partie parties[MAX_PARTIES];

pthread_mutex_t mutex_parties[MAX_PARTIES];
pthread_mutex_t mutex_place = PTHREAD_MUTEX_INITIALIZER;
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
  pthread_mutex_lock(&mutex_place);

  for (int i = 0; i < nb_partie; i++) {
    if (parties[i].mode_jeu != -1) {
      if (parties[i].mode_jeu == mode_jeu &&
          parties[i].nb_joueurs < MAX_CLIENTS) {
        add_player(&parties[i], client_socket);
        pthread_mutex_unlock(&mutex_place);
        return i;
      }
    }
  }

  if (nb_partie < MAX_PARTIES) {
    add_partie(client_socket, mode_jeu);
    nb_partie++;
  }

  pthread_mutex_unlock(&mutex_place);
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


void *handle_tchat_clientX(void *arg) {
  data *d = (data *)arg;

  Partie *partie = &parties[d->index_partie];
  int x = d->index_partie;
  TchatMessage msg;
  while (!(parties[x].players[0].dead) || !(parties[x].players[1].dead) || !(parties[x].players[2].dead) || !(parties[x].players[3].dead) ) {
    memset(&msg, 0, sizeof(TchatMessage));
    if (recv(partie->clients_socket_tcp[d->id], &msg, sizeof(TchatMessage), 0) <
        0) {
      perror("La réception du message a échoué");
      free(arg);
      exit(EXIT_FAILURE);
    }

    bool sept = (msg.entete.CODEREQ == 7);
    msg.entete.CODEREQ = (sept) ? 13 : 14;
    if (sept) { // à tout le monde
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (send(partie->clients_socket_tcp[i], &msg, sizeof(TchatMessage), 0) <
            0) {
          perror("L'envoi du message a échoué");
           free(arg);
          exit(EXIT_FAILURE);
        }
      }
    } else { // à son mate, déjà vérif s'il a un mate
      int mate = (msg.entete.ID + 1) % 4;
      if (send(partie->clients_socket_tcp[mate], &msg, sizeof(TchatMessage), 0) <
          0) {
        perror("L'envoi du message a échoué");
         free(arg);
     
        exit(EXIT_FAILURE);
      }
    }
  }

  if(DEBUG){
    printf(" Fin gestion du client ");
  }

  free(arg);
  return NULL;
}

// Partie incomplete puisque il faut 4 threads pour les 4 joueurs
void *handle_tchat(void *arg) {
  pthread_t thread_tchat_clients[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; i++) {
    data *x = malloc(sizeof(data));
    x->index_partie = *(int *)arg;
    x->id = i;

    pthread_create(&thread_tchat_clients[i], NULL, handle_tchat_clientX, x);
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    pthread_join(thread_tchat_clients[i], NULL);
  }

  if(DEBUG){
    printf(" Fin chat ");
  }
  free(arg);
  return NULL;
}

void init_gridData(int index_partie) {
    GridData *grid = &parties[index_partie].grid;

    memset(&grid->entete, 0, sizeof(grid->entete));
    grid->entete.CODEREQ = 11;
    grid->NUM = 0; // Car premier message de la partie

    grid->width = FIELD_WIDTH + 2 ; // 2 lignes de "-" + 1 ligne de chat
    grid->height = FIELD_HEIGHT + 2;   // 2 colonnes de "|"

    memset(grid->cases, CASE_VIDE, MAX_CASES * MAX_CASES * sizeof(u_int8_t));
    grid->cases[0][0] = J0;
    grid->cases[0][FIELD_HEIGHT - 1] = J1;
    grid->cases[FIELD_WIDTH - 1][0] = J2;
    grid->cases[FIELD_WIDTH - 1][FIELD_HEIGHT - 1] = J3;

    setup_wall(&parties[index_partie]);
}

void *handle_partie(void *arg) {
  int index_partie = *(int *)arg;

  clear_term();
  printf("\033[90mGOOOOOOO\nLa partie commence !!\033[0m\n\n\n\n");

  Partie *partie = &parties[index_partie];

  partie->players[0].p.x = 0;
  partie->players[0].p.y = 0;
  partie->players[1].p.x = 0;
  partie->players[1].p.y = FIELD_HEIGHT - 1;
  partie->players[2].p.x = FIELD_WIDTH - 1;
  partie->players[2].p.y = 0;
  partie->players[3].p.x = FIELD_WIDTH - 1;
  partie->players[3].p.y = FIELD_HEIGHT - 1;

  init_gridData(index_partie);

  if (sendto(partie->send_sock, &partie->grid, sizeof(GridData), 0,
             (struct sockaddr *)&partie->multicast_addr,
             sizeof(partie->multicast_addr)) < 0) {
    perror("L'envoi de la grille a échoué");
    free(arg); 
    exit(EXIT_FAILURE);
  }

//  pthread_t thread_grid;
//  int *x = malloc(sizeof(int));
//  *x = index_partie;
//  pthread_create(&thread_grid, NULL, game_communication, x);

  pthread_t thread_grid_freq;
  int *y = malloc(sizeof(int));
  *y = index_partie;
  pthread_create(&thread_grid_freq, NULL, game_communication_freq, y);

  pthread_t thread_tchat_communication;
  int *z = malloc(sizeof(int));
  *z = index_partie;
  pthread_create(&thread_tchat_communication, NULL, handle_tchat, z);

  //pthread_join(thread_grid, NULL);
  pthread_join(thread_grid_freq, NULL);
  pthread_join(thread_tchat_communication, NULL);

  if(DEBUG){
    printf(" Lancement de toute les gestions de parties partie : %d ",index_partie );
  }
  free(arg);
  return NULL;
}

void *handle_client(void *arg) {
  int client_socket = *(int *)arg;

  EnteteMessage received_message;
  memset(&received_message, 0, sizeof(EnteteMessage));

  // Réception du message depuis le client
  if (recv(client_socket, &received_message, sizeof(EnteteMessage), 0) < 0) {
    perror("La réception du message a échoué");
     free(arg);
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
     free(arg);
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

  pthread_mutex_lock(&mutex_parties[index_partie]);
  printf("Il reste %d places dans la partie n.%d %s\n",
         MAX_CLIENTS - parties[index_partie].nb_joueurs, index_partie,
         (MAX_CLIENTS - parties[index_partie].nb_joueurs == 0)
             ? "\n\033[31m\nNous sommes au complet, la partie peut commencer "
               "!!\033[0m\n"
             : "\n");

  if (MAX_CLIENTS - parties[index_partie].nb_joueurs == 0) {
    pthread_t thread_partie;
    int *x = malloc(sizeof(int));
    if(x==NULL){
      perror("Erreur debut de partie");
      free(arg);
      exit(EXIT_FAILURE);
    }
    *x = index_partie;
    pthread_create(&thread_partie, NULL, handle_partie, x);
    pthread_join(thread_partie, NULL);
  }
  else pthread_mutex_unlock(&mutex_parties[index_partie]);

  free(arg);
  return NULL;
}

void *game_communication_freq(void *arg) {
  int partie_id = *(int *)arg;

  FreqGrid freq_grid;
  memset(&freq_grid, 0, sizeof(FreqGrid));
  time_t start_time, current_time = 0;

  time(&start_time);
  freq_grid.entete.CODEREQ = 12;
  freq_grid.entete.ID = 0;
  freq_grid.entete.EQ = 0;

  GameMessage game_message;

  while (!(parties[partie_id].players[0].dead) || !(parties[partie_id].players[1].dead) || !(parties[partie_id].players[2].dead) || !(parties[partie_id].players[3].dead) ) {
    time(&current_time);

    freq_grid.NUM = 0;
    freq_grid.NB = 0;
    memset(&freq_grid.DATA, 0, sizeof(freq_grid.DATA));

    memset(&game_message, 0, sizeof(GameMessage));

    if (recv(parties[partie_id].send_sock, &game_message, sizeof(GameMessage),
             0) < 0) {
      perror("La réception de la grille a échoué");
      free(arg);
      exit(EXIT_FAILURE);
    }

    if (check_maj(&game_message, &parties[partie_id], &freq_grid) == -1)
      continue;

    freq_grid.NB = freq_grid.NB / 3;
    time(&current_time);
    if (difftime(current_time, start_time) >= 1.0) { // Toutes les secondes
      if (sendto(parties[partie_id].send_sock, &parties[partie_id].grid,
                 sizeof(GridData), 0,
                 (struct sockaddr *)&parties[partie_id].multicast_addr,
                 sizeof(parties[partie_id].multicast_addr)) < 0) {
        perror("L'envoi de la grille a échoué");
        free(arg);
        exit(EXIT_FAILURE);
      }
      start_time = current_time;
    }else {
      if (sendto(parties[partie_id].send_sock, &freq_grid,
                 sizeof(FreqGrid), 0,
                 (struct sockaddr *)&parties[partie_id].multicast_addr,
                 sizeof(parties[partie_id].multicast_addr)) < 0) {
        perror("L'envoi de la grille a échoué");
        free(arg);
        exit(EXIT_FAILURE);
      }
      usleep(10000); // 100ms
    }
  }
  if(DEBUG){
    printf(" Fin de la partie %d le gagnant est : ",partie_id );
  }

  free(arg);
  return NULL;
}

void *game_communication(void *arg) {
  int partie_id = *(int *)arg;

  while (!(parties[partie_id].players[0].dead) || !(parties[partie_id].players[1].dead) || !(parties[partie_id].players[2].dead) || !(parties[partie_id].players[3].dead) ) {

    if (sendto(parties[partie_id].send_sock, &parties[partie_id].grid,
               sizeof(GridData), 0,
               (struct sockaddr *)&parties[partie_id].multicast_addr,
               sizeof(parties[partie_id].multicast_addr)) < 0) {
      perror("L'envoi de la grille a échoué");
      free(arg);
      exit(EXIT_FAILURE);
    }
    sleep(1);
  }
  if(DEBUG){
    printf(" Fin de la partie %d le gagnant est : ",partie_id );
  }

  free(arg);
  return NULL;
}

void init_mutex() {
  for (int i = 0; i < MAX_PARTIES; ++i) {
    pthread_mutex_init(&mutex_parties[i], NULL);
  }
}