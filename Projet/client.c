#include "client.h"
#include "grid.h"

int player_id; // id du joueur
int team_number;
int tcp_socket; // socket pour la connexion TCP avec la partie
char *color;
int game_mode;

GridData game_grid;

int udp_socket; // socket pour la connexion UDP avec la partie
struct sockaddr_in6 udp_listen_addr; // adresse du client en UDP

struct sockaddr_in6 diffuseur_addr; // adresse de la partie en UDP
socklen_t difflen = sizeof(diffuseur_addr);

void receive_gmsg(int client_socket) {
  GameMessage received_message;
  memset(&received_message, 0, sizeof(GameMessage));
  // Réception du message depuis le client
  if (recv(client_socket, &received_message, sizeof(GameMessage), 0) < 0) {
    perror("La réception du message a échoué");
    exit(EXIT_FAILURE);
  }
}

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

void grid_communication() {
  GridData grid;
  while (1) {
    memset(&grid, 0, sizeof(GridData));
    if (recvfrom(udp_socket, &grid, sizeof(GridData) - 1, 0,
                 (struct sockaddr *)&diffuseur_addr, &difflen) < 0) {
      perror("echec de read");
    }

    // print_grid(me, &grid);
  }
}
 

// A faire dans le switch, afin de savoir si on envoie en TCP ou en UDP
void *receive_tchat(void *arg) {
  char message[SIZE_MSG];
  while (1) {
    memset(message, 0, SIZE_MSG);
    if (recv(tcp_socket, message, SIZE_MSG, 0) < 0) {
      perror("La réception du message a échoué");
      exit(EXIT_FAILURE);
    }

    printf("\033[1;32m%s\033[0m\n", message);
  }
}

void *send_tchat(void *arg) {
  char instruction[SIZE_MSG];
  sprintf(instruction, "Veuillez choisir un mode : 1 or 2\n"
                       "      1. Message à tout le monde.\n"
                       "      2. Message à votre partenaire.\n\n"
                       "Mon choix est : ");
  printf("%s", instruction);

  char buf[SIZE_MSG];
  while (1) {
    memset(buf, 0, SIZE_MSG);

    scanf("%s", buf);

    if (atoi(buf) == 1 || atoi(buf) == 2) {
      int choice = atoi(buf);
      char message[SIZE_MSG];
      printf("Entrez votre message : ");
      scanf("%s", message);

      TchatMessage tchat_message;
      memset(&tchat_message, 0, sizeof(TchatMessage));
      tchat_message.entete.CODEREQ = htons(choice + 6);
      tchat_message.entete.ID = htons(player_id);
      tchat_message.entete.EQ = (game_mode == 2) ? team_number : -1;
      tchat_message.LEN = strlen(message);
      strcpy(tchat_message.DATA, message);

      if (send(tcp_socket, &tchat_message, sizeof(TchatMessage), 0) < 0) {
        perror("L'envoi du message a échoué");
        exit(EXIT_FAILURE);
      }
    } else {
      puts("\n\n\033[31mVeuillez choisir 1 (Message à tout le monde) ou 2 "
           "(Message à partenaire)\033[0m\n");
      printf("%s", instruction);
    }
  }
}

void send_recv_tchat() {
  pthread_t thread_recv_tchat;
  pthread_t thread_send_tchat;

  if (pthread_create(&thread_recv_tchat, NULL, receive_tchat, NULL) != 0) {
    perror("Erreur lors de la création du thread pour la réception du tchat");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&thread_send_tchat, NULL, send_tchat, NULL) != 0) {
    perror("Erreur lors de la création du thread pour l'envoi du tchat");
    exit(EXIT_FAILURE);
  }
}

void launch_game() {
  GridData first_grid;
  memset(&first_grid, 0, sizeof(GridData));

  if (recv(udp_socket, &first_grid, sizeof(GridData), 0) < 0) {
    perror("La réception de la première grille a échoué");
    exit(EXIT_FAILURE);
  }

  init_grid(first_grid, player_id);

  // grid_communication();
 }

int main() {
  connexion_to_tcp_server();

  choose_game_mode();

  suscribe_multicast();

  im_ready(); // dernière étape avant de commencer la partie

  launch_game();
}