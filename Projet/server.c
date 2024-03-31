#include "server.h"

#define MAX_PARTIES 4

Partie parties[MAX_PARTIES];

int nb_partie = 0;

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

void init_parties() {
  for (int i = 0; i < MAX_PARTIES; i++) {
    parties[i].partie_id = i;
    parties[i].mode_jeu = -1; // Utilisez -1 ou toute autre valeur invalide pour
                              // indiquer qu'une partie n'est pas active
    parties[i].nb_joueurs = 0;
    parties[i].listen_sock =
        -1; // Socket non valide pour indiquer qu'aucune n'est encore attribuée
    parties[i].send_sock =
        -1; // Socket non valide pour indiquer qu'aucune n'est encore attribuée

    memset(&parties[i].partie_addr, 0, sizeof(parties[i].partie_addr));
    memset(&parties[i].multicast_addr, 0, sizeof(parties[i].multicast_addr));
  }
}

void *handle_partie(void *arg) {
  Partie *partie = (Partie *)arg;
  char buf[SIZE_MSG];

  while (1) {
    memset(buf, 0, sizeof(buf));
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
    puts("Message envoyé");
  }
}

void add_partie(int client_socket, int mode_jeu, int index_partie) {
  parties[index_partie].mode_jeu = mode_jeu;

  parties[index_partie].listen_sock = client_socket;

  parties[index_partie].partie_addr.sin6_family = AF_INET6;
  parties[index_partie].partie_addr.sin6_port = htons(TCP_PORT + index_partie);
  inet_pton(AF_INET6, "::1", &parties[index_partie].partie_addr.sin6_addr);

  init_multicast_socket(&parties[index_partie]);

  add_player(&parties[index_partie]);

  // pthread_create(&parties[nb_partie].thread_partie, NULL, handle_partie,
  //                (void *)&parties[nb_partie]);
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
  char multicast_group[INET6_ADDRSTRLEN];
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

void add_player(Partie *partie) {
  partie->nb_joueurs++;
  //send_suscribe_info(partie->nb_joueurs, partie->listen_sock,
  //                   partie->partie_id);
  
  send_game_s_info(partie->listen_sock,partie->nb_joueurs,partie->partie_id);
}

int join_or_create(int client_socket, int mode_jeu) {
  pthread_mutex_lock(&mutex_partie);

  for (int i = 0; i < nb_partie; i++) {
    if (parties[i].mode_jeu != -1) {
      if (parties[i].mode_jeu == mode_jeu && parties[i].nb_joueurs < 4) {
        add_player(&parties[i]);
        pthread_mutex_unlock(&mutex_partie);
        return i;
      }
    }
  }

  if (nb_partie < MAX_PARTIES) {
    add_partie(client_socket, mode_jeu, nb_partie);
    nb_partie++;
  } else {
    // envoyer un signal pour que y a plus de place dans la partie
  }

  pthread_mutex_unlock(&mutex_partie);
  return nb_partie - 1; // retourne l'index de la partie
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

char *get_color(int x) {
  switch (x % 4) {
  case 0:
    return "yellow";
  case 1:
    return "green";
  case 2:
    return "cyan";
  case 3:
    return "magenta";
  }

  return "white";
}

// Fonction pour envoyer les informations de la partie au joueur
void send_game_s_info(int client_socket, int id_player, int id_partie ) {
    ServerMessage server_message;
    server_message.code_req = (parties[id_partie].mode_jeu==1) ? 9 : 10;

    server_message.id = (id_player * id_partie) + parties[id_partie].nb_joueurs;
    int team_number;
    if (parties[id_partie].mode_jeu == 1) {
      team_number = 1;
    } else {
      team_number = (server_message.id % 2) + 1;
    }
    server_message.eq = team_number;
    server_message.port_udp = parties[id_partie].partie_addr.sin6_port;
    server_message.port_m_diff = parties[id_partie].multicast_addr.sin6_port; 
    
    inet_ntop(AF_INET6, &(parties[id_partie].multicast_addr.sin6_addr),
            server_message.adr_m_diff, INET6_ADDRSTRLEN);

    // Affichage des informations envoyées
    // printf("Informations envoyées au client :\n");
    // printf("Code de la requête : %d\n", server_message.code_req);
    // printf("ID du joueur : %d\n", server_message.id);
    // printf("Équipe du joueur : %d\n", server_message.eq);
    // printf("Port UDP du serveur : %d\n", server_message.port_udp);
    // printf("Port de multidiffusion du serveur : %d\n", server_message.port_m_diff);
    // printf("Adresse de multidiffusion du serveur : %s\n", server_message.adr_m_diff);

    if (send(client_socket, &server_message, sizeof(ServerMessage), 0) < 0) {
        perror("L'envoi des informations de la partie a échoué");
        exit(EXIT_FAILURE);
    }
    printf("Les informations pour le jeu ont été envoyé aux clients \n");
}

void* handle_client(void *arg) {
    int client_socket = *(int *)arg;

    GameMessage received_message;
    // Réception du message depuis le client
    if (recv(client_socket, &received_message, sizeof(GameMessage), 0) < 0) {
        perror("La réception du message a échoué");
        exit(EXIT_FAILURE);
    }
    printf("Message reçu :\n");
    printf("CODEREQ : %d !!\n", received_message.CODEREQ);

    int index_partie = join_or_create(client_socket,received_message.CODEREQ);
   
    char buf[SIZE_MSG];
    memset(buf, 0, sizeof(buf));

    if (recv(parties[index_partie].listen_sock, buf, SIZE_MSG, 0) < 0) {
      perror("La réception de l'adresse et du port UDP a échoué");
      exit(EXIT_FAILURE);
    }

    int id_player, team_number;

    sscanf(buf, "%d,%d", &id_player, &team_number);
    printf("Le joueur %d de la partie n.%d a rejoint l'équipe %d ET EST dès à "
         "présent pret a commencer une partie.\n\n",
         id_player, index_partie, team_number);
  return NULL;
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

  printf("Serveur démarré, en attente de connexions...\n\n");

  // Acceptation des connexions 

  socklen_t addr_len = sizeof(client_addr);
  while (1) {
    tab_socket[nb_partie] = malloc(sizeof(int));
    *(tab_socket[nb_partie]) =
        accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

    if (*(tab_socket[nb_partie]) >= 0) {
      printf("Connexion acceptée\n\n");
      pthread_t thread_client;
      pthread_create(&thread_client, NULL, handle_client,
                     tab_socket[nb_partie]);
    }
  }

  close(server_socket);

  return 0;
}
