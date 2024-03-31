#include "client.h"
#include "data.h"

int player_id;
int team_number;
int tcp_socket; // socket pour la connexion TCP avec la partie
char *color;

int udp_socket; // socket pour la connexion UDP avec la partie
struct sockaddr_in6 udp_send_addr;   // adresse de la partie en UDP
struct sockaddr_in6 udp_listen_addr; // adresse de la partie en UDP

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
  puts("Please select a game mode: 1 or 2");
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

  if (send(tcp_socket, &game_mode, sizeof(game_mode), 0) < 0) {
    perror("L'envoi du mode de jeu a échoué");
    exit(EXIT_FAILURE);
  }
}

void *handle_udp(void *arg) {
  int from_server = *(int *)arg;
  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));

  sprintf(buf, "%sJoueur %d prêt pour la partie%s", color, player_id,
          "\033[0m");
  if (sendto(udp_socket, buf, strlen(buf), 0, (struct sockaddr *)&udp_send_addr,
             sizeof(udp_send_addr)) < 0) {
    perror("L'envoi du message de prêt a échoué");
    exit(EXIT_FAILURE);
  }

  if (team_number == 1) {
    printf("%sJe suis le joueur %d, et je suis prêt pour le 1v3%s\n", color,
           player_id, "\033[0m");
  } else {
    printf("%sJe suis le joueur %d, et je suis dans l'équipe %d%s\n", color,
           player_id, team_number, "\033[0m");
  }

  while (1) {
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

void suscribe_multicast() {
  // Dans un premier temps, on récupère l'adresse et le port de la partie en UDP
  // nécessaire à la connextion multicast en UDP
  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));
  memset(&udp_send_addr, 0, sizeof(udp_send_addr));
  memset(&udp_listen_addr, 0, sizeof(udp_listen_addr));
  int multicast_port;
  int partie_port;
  char multicast_addr[INET6_ADDRSTRLEN];

  ssize_t received = recv(tcp_socket, buf, SIZE_MSG, 0);
  if (received < 0) {
    perror("La réception de l'adresse et du port UDP a échoué");
    exit(EXIT_FAILURE);
  } else if (received == 0) {
    printf("La connexion TCP a été fermée par le serveur.\n");
    exit(EXIT_SUCCESS); // Ou gérer la fermeture selon l'application
  } else {
    if (sscanf(buf, "%d,%d,%d,%d,%s,%s", &player_id, &team_number,
               &multicast_port, &partie_port, multicast_addr, color) != 6) {
      fprintf(stderr, "Erreur lors de l'extraction des informations\n");
      exit(EXIT_FAILURE);
    }
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
      "en0"); // "en0", c pour les macOs, donc pour windows ce serait "eth0"
  // int ifindex = if_nametoindex("eth0"); // "eth0", c pour les linux, donc
  // pour windows ce serait "en0"
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
}

int main() {
  connexion_to_tcp_server();

  sleep(1); // attente pour laisser le temps au serveur de se préparer

  choose_game_mode();

  suscribe_multicast();

  puts("YOUHOU ENFIN LAAAAAA !!!!");

  return 0;
}