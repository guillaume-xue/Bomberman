#include "client.h"
#include "data.h"

enum colors color;

char *color_to_string(enum colors color) {
    switch (color) {
        case MAGENTA:
            return "\033[31m";
        case GREEN:
            return "\033[32m";
        case CYAN:
            return "\033[34m";
        case YELLOW:
            return "\033[33m";
        default:
            return "\033[0m"; // Reset color
    }

    return "\033[0m"; // Reset color
}

int connexion_to_tcp_server() {
  int client_socket = socket(AF_INET6, SOCK_STREAM, 0);
  if (client_socket == -1) {
    perror("La création de la socket a échoué");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in6 server_addr;
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(TCP_PORT);
  inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

  if (connect(client_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("La connexion au serveur a échoué");
    exit(EXIT_FAILURE);
  }

  return client_socket;
}

void choose_game_mode(int client_socket) {
  int game_mode;
  puts("Please select a game mode: 1 or 2");
  puts("      1. Multiplayer");
  puts("      2. Singleplayer");
  printf("\nMon choix est : ");

  char buf[20];
  while (1) {
    scanf("%s", buf);

    if (atoi(buf) == 1 || atoi(buf) == 2) {
      game_mode = atoi(buf);
      break;
    } else {
      puts("Invalid game mode. Please select 1 (Multiplayer) or 2 "
           "(Singleplayer)");
    }
  }

  if (send(client_socket, &game_mode, sizeof(game_mode), 0) < 0) {
    perror("L'envoi du mode de jeu a échoué");
    exit(EXIT_FAILURE);
  }
}

int connect_to_udp_server(int client_socket) {
  int udp_socket;
  struct sockaddr_in6 udp_server_addr;
  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));

  if (recv(client_socket, buf, SIZE_MSG, 0) < 0) {
    perror("La réception de l'adresse et du port UDP a échoué");
    exit(EXIT_FAILURE);
  }

  char *address = strtok(buf, ",");
  int port = atoi(strtok(NULL, ","));

  udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
  if (udp_socket == -1) {
    perror("La création de la socket UDP a échoué");
    exit(EXIT_FAILURE);
  }

  memset(&udp_server_addr, 0, sizeof(udp_server_addr));
  udp_server_addr.sin6_family = AF_INET6;
  udp_server_addr.sin6_port = htons(port);
  inet_pton(AF_INET6, address, &udp_server_addr.sin6_addr);

  if (bind(udp_socket, (struct sockaddr *)&udp_server_addr,
           sizeof(udp_server_addr)) < 0) {
    perror("La liaison de la socket UDP a échoué");
    exit(EXIT_FAILURE);
  }

  // int ifindex = if_nametoindex("eth0");
  int ifindex = if_nametoindex("en0");
  if (ifindex == 0) {
    perror("if_nametoindex failed");
    exit(EXIT_FAILURE);
  }
  struct ipv6_mreq group;
  inet_pton(AF_INET6, address, &group.ipv6mr_multiaddr);
  group.ipv6mr_interface = ifindex;

  if (setsockopt(udp_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group,
                 sizeof(group)) < 0) {
    perror("L'abonnement au groupe multicast a échoué");
    exit(EXIT_FAILURE);
  }

  puts("Waiting for multicast message...");
  memset(buf, 0, sizeof(buf));
  if (read(udp_socket, buf, SIZE_MSG) < 0) {
    perror("La réception du message multicast a échoué");
    exit(EXIT_FAILURE);
  }
  printf("Message multicast reçu: %s\n", buf);

  return udp_socket;
}

int main() {
  int client_socket = connexion_to_tcp_server();

  sleep(1); // attente pour laisser le temps au serveur de se préparer

  choose_game_mode(client_socket);

  connect_to_udp_server(client_socket);

  puts("YOUHOU ENFIN LAAAAAA !!!!");

  return 0;
}