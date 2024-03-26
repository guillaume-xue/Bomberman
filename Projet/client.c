#include "client.h"
#include "data.h"

int main() {
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

  int game_mode;
  puts("Please select a game mode: 1 or 2\n1. Multiplayer\n2. Singleplayer\n");

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

  // PARTIE : Multicast
//   int fd = socket(AF_INET6, SOCK_DGRAM, 0);
//   if (fd < 0) {
//     perror("socket");
//     exit(EXIT_FAILURE);
//   }

//   struct sockaddr_in6 addr;
//   memset(&addr, 0, sizeof(addr));
//   addr.sin6_family = AF_INET6;
//   addr.sin6_addr = in6addr_any;
//   addr.sin6_port = htons(

//   if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
//     perror("bind");
//     exit(EXIT_FAILURE);
//   }

//   struct ipv6_mreq group;
//   inet_pton(AF_INET6, "ff02::1:ffXX", &group.ipv6mr_multiaddr);
//   group.ipv6mr_interface = 0;
//   if (setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group)) <
//       0) {
//     perror("setsockopt");
//     exit(EXIT_FAILURE);
//   }

  return 0;
}