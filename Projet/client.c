#include "client.h"
#include "grid.h"

int player_id;
int team_number;
int tcp_socket; // socket pour la connexion TCP avec la partie
char *color;
int game_mode;
// char* group_c = "239.255.255.250";
GameMessage received_message;

int udp_socket; // socket pour la connexion UDP avec la partie
struct sockaddr_in6 udp_send_addr;   // adresse de la partie en UDP
struct sockaddr_in6 udp_listen_addr; // adresse du client en UDP

void print_udp_subscription(int sockfd) {
  struct ipv6_mreq mreq;
  socklen_t len = sizeof(mreq);

  if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &mreq, &len) == 0) {
    char multicast_group[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(mreq.ipv6mr_multiaddr), multicast_group,
              INET6_ADDRSTRLEN);
    printf("Le socket est abonné au groupe multicast : %s\n", multicast_group);
  } else {
    perror("getsockopt(IPV6_JOIN_GROUP) failed");
  }
}

void receive_gmsg(int client_socket) {
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

  GameMessage request;
  memset(&request, 0, sizeof(GameMessage));
  request.CODEREQ = game_mode;

  if (send(tcp_socket, &request, sizeof(GameMessage), 0) < 0) {
    perror("L'envoi de la demande de jeu a échoué");
    exit(EXIT_FAILURE);
  }
}

void suscribe_multicast() {
  // Dans un premier temps, on récupère l'adresse et le port de la partie en UDP
  // nécessaire à la connexion multicast en UDP

  // char buf[SIZE_MSG];
  ServerMessage serv_message;

  memset(&serv_message, 0, sizeof(ServerMessage));
  memset(&udp_send_addr, 0, sizeof(udp_send_addr));
  memset(&udp_listen_addr, 0, sizeof(udp_listen_addr));

  int multicast_port;
  int partie_port;
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

    partie_port = serv_message.port_udp;

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
  adr.sin6_port = htons(multicast_port);

  if (bind(udp_socket, (struct sockaddr *)&adr, sizeof(adr))) {
    perror("echec de bind");
    close(udp_socket);
  }

  /* initialisation de l'interface locale autorisant le multicast IPv6 */
  int ifindex = if_nametoindex("wlp0s20f3");
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

  char debug_address[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &group.ipv6mr_multiaddr, debug_address, INET6_ADDRSTRLEN);
  printf("Multicast Address: %s Port: %d Scope ID: %d\n", debug_address,
         ntohs(multicast_port), udp_send_addr.sin6_scope_id);

  if (setsockopt(udp_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group,
                 sizeof group) < 0) {
    perror("echec de abonnement groupe");
    close(udp_socket);
    exit(EXIT_FAILURE);
  }
  
  // udp_send_addr.sin6_family = AF_INET6;
  // udp_send_addr.sin6_port = htons(partie_port);
  // inet_pton(AF_INET6, multicast_addr, &udp_send_addr.sin6_addr);

}

void init_players_info(player **players) {
  players = malloc(4 * sizeof(player *));
  for (int i = 0; i < 4; i++) {
    players[i] = malloc(sizeof(player));
    players[i]->p = malloc(sizeof(pos));
    players[i]->id = i + 1;
    players[i]->b = malloc(sizeof(bomb) + 1);
    players[i]->b->set = false;
    players[i]->gmsg = malloc(sizeof(GameMessage));
    players[i]->gmsg->ACTION = 5151;
    players[i]->action = NONE;
  }
  players[player_id]->gmsg = malloc(sizeof(GameMessage));
}

void update_players_action(player **players) {
  if (players[received_message.ID]->id != player_id) {
    players[received_message.ID]->action = received_message.ACTION;
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
}

void wait_for_game_start() {

  // on peut lire avec read
  char buf[10];
  memset(buf, 0, sizeof(buf));
  if (read(udp_socket, buf, sizeof(buf) - 1) < 0) {
    perror("echec de read");
  }
  printf("Message : %s\n", buf);

  // ou avec recv ou recvfrom
  struct sockaddr_in6 diffadr;
  int recu;
  socklen_t difflen = sizeof(diffadr);

  memset(buf, 0, sizeof(buf));
  if ((recu = recvfrom(udp_socket, buf, sizeof(buf) - 1, 0,
                       (struct sockaddr *)&diffadr, &difflen)) < 0) {
    perror("echec de recvfrom.");
  }

  printf("Message : %s\n", buf);
  printf("\t de ");

  // char buffer[SIZE_MSG];
  // struct sockaddr_in6 partie_addr;
  // socklen_t addr_len = sizeof(partie_addr);

  // print_udp_subscription(udp_socket);
  // printf("En attente du début de la partie...\n");

  // while (1) {
  //     // Réception du message de début de partie en multicast
  //     memset(buffer, 0, sizeof(buffer));
  //     if (recvfrom(udp_socket, buffer, sizeof(buffer), 0,
  //                  (struct sockaddr *)&partie_addr, &addr_len) < 0) {
  //         perror("La réception du message de début de partie en multicast a
  //         échoué"); exit(EXIT_FAILURE);
  //     }

  //     // Vérification du contenu du message
  //     if (strcmp(buffer, "La partie a commencé !") == 0) {
  //         printf("La partie a commencé !\n");
  //         break; // Sortir de la boucle lorsque la partie commence
  //     }
  // }
}

int main() {
  connexion_to_tcp_server();

  choose_game_mode();

  suscribe_multicast();

  im_ready(); // dernière étape avant de commencer la partie

  wait_for_game_start();

  printf("La partie a commencé, à vous de jouer !\n");
}