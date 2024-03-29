#include "server.h"

#define MAX_PARTIES 4

// variables globales
Partie parties[MAX_PARTIES];
int index_partie = 0;
// variables globales

void add_player(int p_id, int t_num, int sock_c, int index) {
  int x = parties[index].nb_joueurs;

  parties[index].joueurs[x].player_id = p_id;
  parties[index].joueurs[x].team_number = t_num;
  parties[index].joueurs[x].socket_client = sock_c;
  parties[index].joueurs[x].color = get_color(x);
  parties[index].nb_joueurs++;
}

void add_partie(int client_socket, int mode_jeu) {
  parties[index_partie].partie_id = index_partie;
  parties[index_partie].mode_jeu = mode_jeu;
  add_player((index_partie * 4), mode_jeu, client_socket,
             index_partie); // fois 4 pour avoir des id uniques
  parties[index_partie].nb_joueurs = 1;
  parties[index_partie].port_udp = UDP_PORT + index_partie;
  snprintf(parties[index_partie].adresse_multicast,
           sizeof(parties[index_partie].adresse_multicast), "ff02::1:ff%02x",
           index_partie + 1);

  index_partie++;
}

void join_or_create(int client_socket, int mode_jeu) {
  for (int i = index_partie; i >= 0; i--) {
    if (parties[i].mode_jeu == mode_jeu && parties[i].nb_joueurs < 4) {
      add_player((i * 4) + parties[i].nb_joueurs, mode_jeu, client_socket, i);
      return;
    }
  }

  add_partie(client_socket, mode_jeu);
//   send_multicast_message(index_partie - 1);
}


void handle_client(int client_socket) {
  int mode_jeu = ask_game_mode(client_socket);

  int tmp = index_partie;
  join_or_create(client_socket, mode_jeu);
  if (tmp != index_partie)
    tmp = index_partie - 1;

  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));

  int our_player = parties[tmp].nb_joueurs - 1;
  snprintf(buf, SIZE_MSG, "%s,%d", parties[tmp].adresse_multicast,
           parties[tmp].port_udp);
}

enum colors get_color(int x) {
  switch (x % 4) {
  case 0:
    return GREEN;
  case 1:
    return YELLOW;
  case 2:
    return MAGENTA;
  case 3:
    return CYAN;

  default:
    return GREEN;
  }
}

int ask_game_mode(int client_socket) {
  int player_choice;

  puts("Attente du choix du joueur pour le mode de jeu");

  if (recv(client_socket, &player_choice, sizeof(player_choice), 0) < 0) {
    perror("La réception de la demande de mode de jeu a échoué");
    exit(EXIT_FAILURE);
  }

  printf("Le joueur a choisi le mode de jeu : `%s`\n",
         player_choice == 1 ? "multiplayer" : "singleplayer");

  return player_choice;
}

int main() {
  int server_socket, client_socket;
  struct sockaddr_in6 server_addr, client_addr;

  // Création de la socket TCP
  server_socket = socket(AF_INET6, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("La création de la socket a échoué");
    exit(EXIT_FAILURE);
  }

  // Initialisation de l'adresse du serveur
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(TCP_PORT);
  inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

  // Lier la socket au port spécifié
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("La liaison a échoué");
    exit(EXIT_FAILURE);
  }

  // Écoute des connexions entrantes
  if (listen(server_socket, 5) < 0) {
    perror("Écoute des connexions a échoué");
    exit(EXIT_FAILURE);
  }

  printf("Serveur démarré, en attente de connexions...\n");

  // Acceptation des connexions
  socklen_t addr_len = sizeof(client_addr);
  while (1) {
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket < 0) {
      perror("L'acceptation de la connexion a échoué");
      exit(EXIT_FAILURE);
    }

    // Traitement du client dans un nouveau thread
    handle_client(client_socket);
    close(client_socket);
  }

  close(server_socket);

  return 0;
}
