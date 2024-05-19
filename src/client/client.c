#include "client.h"
#include "grid.h"

int player_id; // id du joueur
int team_number;
int tcp_socket; // socket pour la connexion TCP avec la partie
char* color;
int game_mode = 0;
GridData game_grid;

int udp_socket; // socket pour la connexion UDP avec la partie
struct sockaddr_in6 udp_listen_addr; // adresse du client en UDP

struct sockaddr_in6 diffuseur_addr; // adresse de la partie en UDP
socklen_t difflen = sizeof(diffuseur_addr);

pthread_mutex_t mutex_partie = PTHREAD_MUTEX_INITIALIZER;

line *l;

void connexion_to_tcp_server() {
  tcp_socket = socket(AF_INET6, SOCK_STREAM, 0);
  if (tcp_socket == -1) {
    perror("La création de la socket a échoué");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in6 server_addr;
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(TCP_PORT);
  inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

  if (connect(tcp_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("La connexion au serveur a échoué");
    exit(EXIT_FAILURE);
  }
}

void choose_game_mode() {
  char instruction[SIZE_MSG];
  sprintf(instruction, "Veuillez choisir un mode : 1 or 2\n      1. 1v3\n      "
                       "2. 2v2\n\nMon choix est : ");
  printf("%s", instruction);

  char buf[20];
  while (1) {
    scanf("%s", buf);

    if (atoi(buf) == 1 || atoi(buf) == 2) {
      game_mode = atoi(buf);
      break;
    } else {
      puts("\n\n\033[31mInvalid game mode. Please select 1 (1v3) or 2 "
           "(2v2)\033[0m\n");
      printf("%s", instruction);
    }
  }

  clear_term();
  EnteteMessage request;
  memset(&request, 0, sizeof(EnteteMessage));
  request.CODEREQ = game_mode;

  if (send(tcp_socket, &request, sizeof(EnteteMessage), 0) < 0) {
    perror("L'envoi de la demande de jeu a échoué");
    exit(EXIT_FAILURE);
  }
}

void suscribe_multicast() {
  // Dans un premier temps, on récupère l'adresse et le port de la partie en UDP
  // nécessaire à la connexion multicast en UDP

  ServerMessage serv_message;

  memset(&serv_message, 0, sizeof(ServerMessage));

  int multicast_port;
  char multicast_addr[INET6_ADDRSTRLEN];

  ssize_t received = recv(tcp_socket, &serv_message, sizeof(ServerMessage), 0);
  if (received < 0) {
    perror("La réception de l'adresse et du port UDP a échoué");
    exit(EXIT_FAILURE);
  } else if (received == 0) {
    printf("La connexion TCP a été fermée par le serveur.\n");
    exit(EXIT_SUCCESS); // Ou gérer la fermeture selon l'application
  } else {
    player_id = serv_message.id;

    team_number = serv_message.eq;

    multicast_port = serv_message.port_m_diff;

    strcpy(multicast_addr, serv_message.adr_m_diff);

    color = id_to_color(player_id);
  }

  if ((udp_socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    perror("echec de socket");
  }

  int ok = 1;
  if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
    perror("echec de SO_REUSEADDR");
    close(udp_socket);
  }

  if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEPORT, &ok, sizeof(ok)) < 0) {
    perror("setsockopt(SO_REUSEPORT) failed");
    exit(EXIT_FAILURE);
  }

  /* Initialisation de l'adresse de reception */
  struct sockaddr_in6 adr;
  memset(&adr, 0, sizeof(adr));
  adr.sin6_family = AF_INET6;
  adr.sin6_addr = in6addr_any;
  adr.sin6_port = multicast_port;

  if (bind(udp_socket, (struct sockaddr *)&adr, sizeof(adr))) {
    perror("echec de bind");
    close(udp_socket);
  }

  /* initialisation de l'interface locale autorisant le multicast IPv6 */
  int ifindex = if_nametoindex("en0");
  if (ifindex == 0)
    perror("if_nametoindex");

  /* s'abonner au groupe multicast */
  struct ipv6_mreq group;
  if (inet_pton(AF_INET6, multicast_addr, &group.ipv6mr_multiaddr) != 1) {
    perror("Erreur de conversion d'adresse multicast");
    close(udp_socket);
    exit(EXIT_FAILURE);
  }
  group.ipv6mr_interface = ifindex;

  if (setsockopt(udp_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group,
                 sizeof group) < 0) {
    perror("echec de abonnement groupe");
    close(udp_socket);
    exit(EXIT_FAILURE);
  }
}

void im_ready() {
  GameMessage ready;
  memset(&ready, 0, sizeof(GameMessage));

  ready.CODEREQ = game_mode + 2;
  ready.ID = player_id;
  ready.EQ = (game_mode == 2) ? team_number : -1;

  if (send(tcp_socket, &ready, sizeof(GameMessage), 0) < 0) {
    perror("L'envoi de la demande de jeu a échoué");
    exit(EXIT_FAILURE);
  }

  if (game_mode == 1)
    printf("Mode de jeu : SOLO.\nJe suis %sJoueur %d%s.\n", color, player_id,
           "\033[0m");
  else
    printf("Mode de jeu : EQUIPE.\nJe suis %sJoueur %d%s dans l'équipe %d.\n",
           color, player_id, "\033[0m", team_number);
}

void *receive_grid(void *arg) {
  ssize_t recu;
  while (1) {
    recu = recv(udp_socket, &game_grid, sizeof(GridData), 0);
    if (recu < 0) {
      perror("La réception de la grille a échoué");
      exit(EXIT_FAILURE);
    }
    else if (recu == 0) {
        printf("Connexion fermée par le serveur.\n");
        break;
    }
    else {
        //alarm(TIMEOUT_SECONDS);
    } 
  }
  //close(udp_socket);
  //exit(EXIT_SUCCESS);
  return NULL;
}

void *receive_tchat(void *arg) {
  TchatMessage tchat_message;
  while (1) {
    int recu = recv(tcp_socket, &tchat_message, sizeof(TchatMessage), 0);
    if (recu <= 0) {
      if (recu < 0) {
        perror("Tchat client : La réception du message a échoué");
      } else {
        printf("Le client s'est déconnecté.\n");
      }
      //close(tcp_socket); 
      pthread_exit(NULL); 
   }

    //Vérification fin de partie 
    if(tchat_message.entete.CODEREQ == 15 || tchat_message.entete.CODEREQ==16){
        if (tchat_message.entete.CODEREQ == 15) {

          printf("Fin de partie en mode solo. Le joueur %d a gagné.\n", tchat_message.entete.ID);
          exit(EXIT_SUCCESS);
        }
        else{
          printf("Fin de partie en mode équipe. L'équipe avec le joueur %d a gagné.\n", tchat_message.entete.ID);
          exit(EXIT_SUCCESS);
        }
    }
    
    // On ajoute le message reçu à la conversation
    if (l->tchatbox.nb_lines == TCHATBOX_HEIGHT - 3) {
      for (int i = 0; i < TCHATBOX_HEIGHT - 3; i++) {
        strcpy(l->tchatbox.conv[i], l->tchatbox.conv[i + 1]);
      }
      snprintf(l->tchatbox.conv[TCHATBOX_HEIGHT - 4], TEXT_SIZE, "%d : %s", tchat_message.entete.ID, tchat_message.DATA + 1);
    } else {
      snprintf(l->tchatbox.conv[l->tchatbox.i], TEXT_SIZE, "%d : %s", tchat_message.entete.ID, tchat_message.DATA + 1);
      l->tchatbox.i++;
      l->tchatbox.nb_lines++;
      eraser(l);
    }
  }
  return NULL;
}


void launch_game() {
  memset(&game_grid, 0, sizeof(GridData));

  if (recvfrom(udp_socket, &game_grid, sizeof(GridData), 0,
               (struct sockaddr *)&diffuseur_addr, &difflen) < 0) {
    perror("La réception de la grille a échoué");
    exit(EXIT_FAILURE);
  }

  l = init_grid(game_grid, player_id, game_mode);
  
  pthread_t thread_recv_grid;
  if (pthread_create(&thread_recv_grid, NULL, receive_grid, NULL) != 0) {
    perror(
        "Erreur lors de la création du thread pour la réception de la grille");
    exit(EXIT_FAILURE);
  }

  pthread_t thread_recv_tchat;
  if (pthread_create(&thread_recv_tchat, NULL, receive_tchat, NULL) != 0) {
    perror("Erreur lors de la création du thread pour la réception du tchat");
    exit(EXIT_FAILURE);
  }


  GameMessage my_action;
  TchatMessage my_msg;
  while (1) {
    ACTION a = control(l);
    if (a != NONE) {
      if (a == QUIT) {
        break;
      } else if (a == SUBMIT) {
        memset(&my_msg, 0, sizeof(TchatMessage));
        my_msg.entete.CODEREQ = atoi(&l->data[0]);
        my_msg.entete.ID = player_id;
        my_msg.entete.EQ = (game_mode == 2) ? team_number : -1;
        my_msg.LEN = strlen(l->data);
        strcpy(my_msg.DATA, l->data);

        if (send(tcp_socket, &my_msg, sizeof(TchatMessage), 0) < 0) {
          perror("L'envoi du message a échoué");
          exit(EXIT_FAILURE);
        }
      } else {
        memset(&my_action, 0, sizeof(GameMessage));
        my_action.CODEREQ = game_mode + 4;
        my_action.ID = player_id;
        my_action.EQ = (game_mode == 2) ? team_number : -1;
        my_action.ACTION = a;

        if (sendto(udp_socket, &my_action, sizeof(GameMessage), 0,
                   (struct sockaddr *)&diffuseur_addr, difflen) < 0) {
          perror("L'envoi de l'action a échoué");
          exit(EXIT_FAILURE);
        }
      }
    }
    usleep(10000); // 10ms
    print_grid(game_grid, l);
  }

  clear_grid();
}

// |---------------------||---------------------||---------------------||---------------------|
// |     Partie n.X      ||     Partie n.X      ||     Partie n.X      ||     Partie n.X      |
// |---------------------||---------------------||---------------------||---------------------|
// |      En cours       ||      En cours       ||      En cours       ||      En cours       |
// |---------------------||---------------------||---------------------||---------------------|
// |   Joueurs présent:  ||   Joueurs présent:  ||   Joueurs présent:  ||   Joueurs présent:  |
// |                     ||                     ||                     ||                     |
// |      joueur 1       ||      joueur 1       ||      joueur 1       ||      joueur 1       |
// |      joueur 2       ||      joueur 2       ||      joueur 2       ||      joueur 2       |
// |      joueur 3       ||      joueur 3       ||      joueur 3       ||      joueur 3       |
// |      joueur 4       ||      joueur 4       ||      joueur 4       ||      joueur 4       |
// |                     ||                     ||                     ||                     |
// |---------------------||---------------------||---------------------||---------------------|