#include "server.h"

Partie parties[MAX_PARTIES];

typedef struct bomb_arg{
    int index_partie;
    int id_player;
} bomb_arg;

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


void *handle_tchat_clientX(void *arg) {
  data *d = (data *)arg;

  Partie *partie = &parties[d->index_partie];

  TchatMessage msg;
  while (1) {
    memset(&msg, 0, sizeof(TchatMessage));
    if (recv(partie->clients_socket_tcp[d->id], &msg, sizeof(TchatMessage), 0) <
        0) {
      perror("La réception du message a échoué");
      exit(EXIT_FAILURE);
    }

    bool sept = (msg.entete.CODEREQ == 7);
    msg.entete.CODEREQ = (sept) ? 13 : 14;
    if (sept) { // à tout le monde
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (send(partie->clients_socket_tcp[i], &msg, sizeof(TchatMessage), 0) <
            0) {
          perror("L'envoi du message a échoué");
          exit(EXIT_FAILURE);
        }
      }
    } else { // à son mate, déjà vérif s'il a un mate
      int mate = (msg.entete.ID + 1) % 4;
      if (send(partie->clients_socket_tcp[mate], &msg, sizeof(TchatMessage), 0) <
          0) {
        perror("L'envoi du message a échoué");
        exit(EXIT_FAILURE);
      }
    }
  }
  free(arg);
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

  free(arg);
  return NULL;
}

int get_grid(int index_partie, int x, int y) {
    return parties[index_partie].grid.cases[x][y];
}

void set_grid(int index_partie, int x, int y, ContenuCase c) {
    parties[index_partie].grid.cases[x][y] = c;
}

void clear_grid(int index_partie, int x, int y) {
    set_grid(index_partie, x, y, CASE_VIDE);
}

bool is_bomb(int index_partie, int x, int y) {
    return get_grid(index_partie, x, y) == BOMBE;
}

// Retourne vrai si la case est un mur cassable
bool is_wall_breakable(int index_partie, int x, int y){
    return get_grid(index_partie, x, y) == MUR_DESTRUCTIBLE;
}

// Retourne vrai si la case est un mur
bool is_wall(int index_partie, int x, int y) {
    return get_grid(index_partie, x, y) == MUR_DESTRUCTIBLE || get_grid(index_partie, x, y) == MUR_INDESTRUCTIBLE;
}

bool is_vide(int index_partie, int x, int y) {
    return get_grid(index_partie, x, y) == CASE_VIDE;
}

bool is_player(int index_partie, int x, int y) {
    for (int i = 5; i <= 8; ++i) {
        if (get_grid(index_partie, x, y) == i) return true;
    }
    return false;
}

bool is_movable(int index_partie, int x, int y) {
    return x >= 0 && x <= parties[index_partie].grid.width - 2 &&
           y >= 0 && y <= parties[index_partie].grid.height - 2 &&
           !is_wall(index_partie, x, y) && !is_bomb(index_partie, x, y) &&
           is_player(index_partie, x, y) == false;
}

bool is_exploding(int index_partie, int x, int y) {
    return get_grid(index_partie, x, y) == EXPLOSION;
}

// Place les murs sur la grille
void setup_wall(int index_partie) {
    // On met des murs incassables sur les cases impaires
    for (int i = 1; i < parties[index_partie].grid.width; i+=2) {
        for (int j = 1; j < parties[index_partie].grid.height; j+=2) {
            set_grid(index_partie, i, j, MUR_INDESTRUCTIBLE);
        }
    }
    // On met des murs cassables aléatoirement
    int i = 0;
    while (i < NB_WALLS) {
        int x = rand() % parties[index_partie].grid.width;
        int y = rand() % parties[index_partie].grid.height;
        if (get_grid(index_partie, x, y) == CASE_VIDE){
            set_grid(index_partie, x, y, MUR_DESTRUCTIBLE);
            i++;
        }
    }
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

   // Méthode simple à voir si on va faire un algorithme de création de labyrinthe
    // for (int x = 0; x < FIELD_WIDTH; x++) {
    //     for (int y = 0; y < FIELD_HEIGHT; y++) {
    //         if ((x % 2 == 0 && y % 2 == 0) &&           
    //         !(x == 0 && (y == 0 || y == FIELD_HEIGHT - 1)) &&  // Exclure le coin en haut à gauche et en bas à gauche
    //         !(x == FIELD_WIDTH - 1 && (y == 0 || y == FIELD_HEIGHT - 1))) { // Exclure le coin en haut à droite et en bas à droite
    //         grid->cases[x][y] = MUR_INDESTRUCTIBLE;
    //     }
    //     }
    // }

    setup_wall(index_partie);
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
    exit(EXIT_FAILURE);
  }

  pthread_t thread_grid;
  int *x = malloc(sizeof(int));
  *x = index_partie;
  pthread_create(&thread_grid, NULL, game_communication, x);

  pthread_t thread_tchat_communication;
  int *y = malloc(sizeof(int));
  *y = index_partie;
  pthread_create(&thread_tchat_communication, NULL, handle_tchat, y);

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

  pthread_mutex_lock(&mutex_partie);
  printf("Il reste %d places dans la partie n.%d %s\n",
         MAX_CLIENTS - parties[index_partie].nb_joueurs, index_partie,
         (MAX_CLIENTS - parties[index_partie].nb_joueurs == 0)
             ? "\n\033[31m\nNous sommes au complet, la partie peut commencer "
               "!!\033[0m\n"
             : "\n");

  if (MAX_CLIENTS - parties[index_partie].nb_joueurs == 0) {
    pthread_t thread_partie;
    int *x = malloc(sizeof(int));
    *x = index_partie;
    pthread_create(&thread_partie, NULL, handle_partie, x);
  } else {
    pthread_mutex_unlock(&mutex_partie);
  }

  free(arg);
  return NULL;
}

// Fonction appelée par le thread pour faire exploser la bombe
void *explose_handler(void * arg) {
    sleep(3);
    int index_partie = ((bomb_arg *)arg)->index_partie;
    int id_player = ((bomb_arg *)arg)->id_player;
    explode_bombe(index_partie, id_player);
    free(arg);
    return NULL;
}

// Fait exploser la bombe
void explode_bombe(int index_partie, int id_player){
    // On met les explosions
    for (int i = parties[index_partie].players[id_player].b.x - 1; i <= parties[index_partie].players[id_player].b.x + 1; i++) {
        for (int j = parties[index_partie].players[id_player].b.y - 1; j <= parties[index_partie].players[id_player].b.y + 1; j++) {
            // On casse les murs cassables
            if (i >= 0 && i < parties[index_partie].grid.width &&
                j >= 0 && j < parties[index_partie].grid.height &&
                (is_wall_breakable(index_partie, i, j) || is_vide(index_partie, i, j))){
                set_grid(index_partie, i, j, EXPLOSION);
            }
            // On tue les joueurs
            if(is_player(index_partie, i, j)){
                int id = get_grid(index_partie, i, j) - 5;
                parties[index_partie].players[id].dead = true;
                set_grid(index_partie, i, j, EXPLOSION);
            }
        }
    }
    sleep(1); // Temps d'affichage de l'explosion
    // On efface la bombe et les explosions
    for (int i = parties[index_partie].players[id_player].b.x - 1; i <= parties[index_partie].players[id_player].b.x + 1; i++) {
        for (int j = parties[index_partie].players[id_player].b.y - 1; j <= parties[index_partie].players[id_player].b.y + 1; j++) {
            if (is_exploding(index_partie, i, j) || is_bomb(index_partie, i, j)){
                clear_grid(index_partie, i, j);
            }
        }
    }
    parties[index_partie].players[id_player].b.set = false;
}

int place_bomb(int index_partie, int id_player, pos p) {
    if (parties[index_partie].players[id_player].b.set == true) return 1; // Si le joueur a déjà une bombe

    parties[index_partie].players[id_player].b.x = p.x;
    parties[index_partie].players[id_player].b.y = p.y;

    pthread_t thread;
    bomb_arg *arg = malloc(sizeof(bomb_arg));
    arg->index_partie = index_partie;
    arg->id_player = id_player;
    parties[index_partie].players[id_player].b.set = true;
    if (pthread_create(&thread, NULL, explose_handler, arg) != 0){
        fprintf(stderr, "Erreur lors de la création du thread.\n");
        return 1;
    }

    return 0;
}

// EN cour , -> version BASIQUE
// On vérifie si l'action du joueur est legit
int check_maj(GameMessage *game_message, Partie *partie) {
    int id = game_message->ID - 1;
    // Si le joueur est mort, on ne fait rien
    if (partie->players[id].dead == true) {
        return -1;
    }
    ACTION action = game_message->ACTION;

    pos p = partie->players[id].p;
    switch (action) {
    case UP:
        if (is_movable(partie->partie_id, p.x, p.y - 1) == false)
            return -1;
        else if (partie->grid.cases[p.x][p.y - 1] == MUR_DESTRUCTIBLE) {
            return -1;
        } else {
            partie->players[id].p.y = p.y - 1;
            partie->grid.cases[p.x][p.y] = CASE_VIDE;
            partie->grid.cases[p.x][p.y - 1] = id + 5;
        }
        break;
    case DOWN:
        if (is_movable(partie->partie_id, p.x, p.y + 1) == false) {
            return -1;
        }else if (partie->grid.cases[p.x][p.y + 1] == MUR_DESTRUCTIBLE) {
           return -1;
        } else {
            partie->players[id].p.y = p.y + 1;
            partie->grid.cases[p.x][p.y] = CASE_VIDE;
            partie->grid.cases[p.x][p.y + 1] = id + 5;
        }
        break;
    case LEFT:
        if (is_movable(partie->partie_id, p.x - 1, p.y) == false)
            return -1;

        else if (partie->grid.cases[p.x - 1][p.y] == MUR_DESTRUCTIBLE) {
          return -1;
        } else {
            partie->players[id].p.x = p.x - 1;
            partie->grid.cases[p.x][p.y] = CASE_VIDE;
            partie->grid.cases[p.x - 1][p.y] = id + 5;
        }
        break;// détecter si un joueur est touché par l'explosion
    case RIGHT:
        if (is_movable(partie->partie_id, p.x + 1, p.y) == false)
            return -1;
        else if (partie->grid.cases[p.x + 1][p.y] == MUR_DESTRUCTIBLE) {
          return -1;
        } else {
            partie->players[id].p.x = p.x + 1;
            partie->grid.cases[p.x][p.y] = CASE_VIDE;
            partie->grid.cases[p.x + 1][p.y] = id + 5;
        }
        break;
    case BOMB:
        if(place_bomb(partie->partie_id, id, p) == 1) return -1;
        break;
    case QUIT:
        break; // à implémenter
    default:
        return -1;
    }
    if (partie->players[id].b.set == true) {
        set_grid(partie->partie_id, partie->players[id].b.x, partie->players[id].b.y, BOMBE);
    }
    return 0;
}


void *game_communication(void *arg) {
  int partie_id = *(int *)arg;

  GameMessage game_message;
  while (1) {

    memset(&game_message, 0, sizeof(GameMessage));
    if (recv(parties[partie_id].send_sock, &game_message, sizeof(GameMessage),
             0) < 0) {
      perror("La réception de la grille a échoué");
      exit(EXIT_FAILURE);
    }

    if (check_maj(&game_message, &parties[partie_id]) == -1)
      continue;

    if (sendto(parties[partie_id].send_sock, &parties[partie_id].grid,
               sizeof(GridData), 0,
               (struct sockaddr *)&parties[partie_id].multicast_addr,
               sizeof(parties[partie_id].multicast_addr)) < 0) {
      perror("L'envoi de la grille a échoué");
      exit(EXIT_FAILURE);
    }

    // sleep(INTERVALLE_ENVOI);
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
      pthread_create(&thread_client, NULL, handle_client, x);
    }
  }

  close(server_socket);

  return 0;
}
