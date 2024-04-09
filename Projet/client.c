#include "client.h"

#include "grid.h"


int player_id;
int team_number;
int tcp_socket; // socket pour la connexion TCP avec la partie
char *color;
GameMessage received_message;

int udp_socket; // socket pour la connexion UDP avec la partie
struct sockaddr_in6 udp_send_addr;   // adresse de la partie en UDP
struct sockaddr_in6 udp_listen_addr; // adresse du client en UDP

// Fonction pour envoyer une demande de jeu au serveur
void send_game_request(int client_socket, int CODEREQ) {
    GameMessage request;
    memset(&request, 0, sizeof(GameMessage));
    request.CODEREQ = CODEREQ;

    printf("CODEREQ : %d \n", request.CODEREQ);
    if (send(client_socket, &request, sizeof(GameMessage), 0) < 0) {
        perror("L'envoi de la demande de jeu a échoué");
        exit(EXIT_FAILURE);
    }
    printf(" Message envoyé pour intégrer ou lancer une partie \n"); 

    // Attente de la réponse du server pour avoir toutes les informations nécessaires sur la partie 
    ServerMessage mess_serv;
    ssize_t bytes_received = recv(client_socket, &mess_serv, sizeof(ServerMessage), 0);

    if (bytes_received < 0) {
        perror("La réception des informations du serveur a échoué");
        exit(EXIT_FAILURE);
    } else if (bytes_received == 0) {
        printf("Le serveur a fermé la connexion\n");
    } else {
        printf("Informations reçues du serveur :\n");
        printf("Code de requête : %d\n", mess_serv.code_req);
        printf("ID : %d\n", mess_serv.id);
        printf("Équipe : %d\n", mess_serv.eq);
        // printf("Port UDP : %d\n", mess_serv.port_udp);
        // printf("Port de multidiffusion : %d\n", mess_serv.port_m_diff);
        // printf("Adresse de multidiffusion : %s\n", mess_serv.adr_m_diff);
    }

    printf("Fin des informations sur server \n\n");
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
  int game_mode;
  puts("Veuillez choisir un mode : 1 or 2");
  puts("      1. 1v3");
  puts("      2. 2v2");
  printf("\nMon choix est : ");

  char buf[20];
  while (1) {
    scanf("%s", buf);

    if (atoi(buf) == 1 || atoi(buf) == 2) {
      game_mode = atoi(buf);
      break;
    } else {
      puts("Invalid game mode. Please select 1 (1v3) or 2 "
           "(2v2)");
    }
  }

  system("clear");

  send_game_request(tcp_socket, game_mode);
}

void *handle_udp(void *arg) {
  int from_server = *(int *)arg;
  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));

  snprintf(buf, SIZE_MSG, "%d,%d", player_id, team_number);
  if (sendto(udp_socket, buf, strlen(buf), 0, (struct sockaddr *)&udp_send_addr,
             sizeof(udp_send_addr)) < 0) {
    perror("L'envoi du message de prêt a échoué");
    exit(EXIT_FAILURE);
  }

  puts("En attente de la partie pour commencer");
  if (team_number == 1) {
    printf("%sJe suis le joueur %d, et je suis prêt pour le 1v3%s\n", color,
           player_id, "\033[0m");
  } else {
    printf("%sJe suis le joueur %d, et je suis dans l'équipe %d%s\n", color,
           player_id, team_number, "\033[0m");
  }

  while (1) {
    memset(buf, 0, sizeof(buf));
    ssize_t received = recv(from_server, buf, SIZE_MSG, 0);
    if (received < 0) {
      perror("La réception de l'adresse et du port UDP a échoué");
      exit(EXIT_FAILURE);
    } else if (received == 0) {
      printf("La connexion UDP a été fermée par le serveur.\n");
      exit(EXIT_SUCCESS); // Ou gérer la fermeture selon l'application
    } else {
      printf("Message reçu : %s\n", buf);
    }
  }
}

char *str_to_color(char *str) {
  if (strcmp(str, "green") == 0) {
    return "\033[0;32m";
  } else if (strcmp(str, "yellow") == 0) {
    return "\033[0;33m";
  } else if (strcmp(str, "magenta") == 0) {
    return "\033[0;35m";
  } else if (strcmp(str, "cyan") == 0) {
    return "\033[0;36m";
  } else {
    return "\033[0m";
  }
}

void suscribe_multicast() {
  // Dans un premier temps, on récupère l'adresse et le port de la partie en UDP
  // nécessaire à la connextion multicast en UDP
  
  //char buf[SIZE_MSG];
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

    //color = str_to_color(token);
  }

  // Maintenant on se connecte en UDP à la partie
  udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
  if (udp_socket == -1) {
    perror("La création de la socket UDP a échoué");
    exit(EXIT_FAILURE);
  }

  int reuse = 1;
  if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    perror("La configuration de la socket UDP a échoué");
    exit(EXIT_FAILURE);
  }

  udp_listen_addr.sin6_family = AF_INET6;
  udp_listen_addr.sin6_port = htons(multicast_port);
  udp_listen_addr.sin6_addr = in6addr_any;

  if (bind(udp_socket, (struct sockaddr *)&udp_listen_addr,
           sizeof(udp_listen_addr)) < 0) {
    perror("La liaison de la socket UDP a échoué");
    exit(EXIT_FAILURE);
  }

  // Abonnement au groupe multicast
  int ifindex = if_nametoindex(
      "en0"); // à check en fonction de la machine sur laquelle on lance,

  if (ifindex == 0) {
    perror("if_nametoindex failed");
    exit(EXIT_FAILURE);
  }

  struct ipv6_mreq group;
  if (inet_pton(AF_INET6, multicast_addr, &group.ipv6mr_multiaddr) != 1) {
    perror("Erreur de conversion de l'adresse multicast");
    exit(EXIT_FAILURE);
  }

  group.ipv6mr_interface = ifindex;

  if (setsockopt(udp_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group,
                 sizeof(group)) < 0) {
    perror("L'abonnement au groupe multicast a échoué");
    exit(EXIT_FAILURE);
  }

  udp_send_addr.sin6_family = AF_INET6;
  udp_send_addr.sin6_port = htons(partie_port);
  inet_pton(AF_INET6, "::1", &udp_send_addr.sin6_addr);

  int *x = malloc(sizeof(int));
  *x = udp_socket;
  pthread_t thread;
  pthread_create(&thread, NULL, handle_udp, (void *)x);
  pthread_join(thread, NULL);
}

void init_players_info(player **players) {
    players = malloc(4 * sizeof(player*));
    for (int i = 0; i < 4; i++) {
        players[i] = malloc(sizeof(player));
        players[i]->p = malloc(sizeof(pos));
        players[i]->id = i+1;
        players[i]->b = malloc(sizeof(bomb)+1);
        players[i]->b->set = false;
        players[i]->gmsg = malloc(sizeof(GameMessage));
        players[i]->gmsg->ACTION = 5151;
        players[i]->action = NONE;
    }
    players[player_id]->gmsg = malloc(sizeof(GameMessage));
}

void update_players_action(player **players){
    if (players[received_message.ID]->id != player_id) {
        players[received_message.ID]->action = received_message.ACTION;
    }
}

int main() {
    //connexion_to_tcp_server();

    //sleep(1); // attente pour laisser le temps au serveur de se préparer

    //choose_game_mode();

    //suscribe_multicast();

    //puts("YOUHOU ENFIN LAAAAAA !!!!");

    player **players;
    init_players_info(players);
    grid_creation(player_id, players);

    return 0;
}