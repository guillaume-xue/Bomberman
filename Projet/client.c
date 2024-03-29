#include "client.h"

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

  // Lancement du mode de jeu après le choix 
  if(game_mode == 1){
     
  }
  else{

  }

  return 0;
}