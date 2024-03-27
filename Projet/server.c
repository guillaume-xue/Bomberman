#include "server.h"

#define MAX_PARTIES 4

// variables globales
Partie parties[MAX_PARTIES];
int index_partie = 0;
// variables globales

int add_player(int p_id, int t_num, int sock_c, int index) {
  int x = parties[index].nb_joueurs;

  parties[index].joueurs[x].player_id = p_id;
  parties[index].joueurs[x].team_number = t_num;
  parties[index].joueurs[x].socket_client = sock_c;
  parties[index].joueurs[x].color = get_color(x);
  parties[index].nb_joueurs++;

  return parties[index].nb_joueurs;
}

int init_multicast_socket() {
  int fd = socket(AF_INET6, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in6 gradr;
  memset(&gradr, 0, sizeof(gradr));
  gradr.sin6_family = AF_INET6;
  inet_pton(AF_INET6, parties[index_partie].adresse_multicast,
            &gradr.sin6_addr);
  gradr.sin6_port = htons(parties[index_partie].port_udp);

  int ifindex = if_nametoindex("en0");
  // int ifindex = if_nametoindex("eth0");

  if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex,
                 sizeof(ifindex)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
 
  return fd;
}

int send_udp_info_to_client(int index_player) {

  int client_socket = parties[index_partie].joueurs[index_player].socket_client;
  char *adresse_multicast = parties[index_partie].adresse_multicast;
  int port_udp = parties[index_partie].port_udp;
  enum colors color = parties[index_partie].joueurs[index_player].color;

  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, SIZE_MSG, "%s,%d,%d", adresse_multicast, port_udp, color);

  if (send(client_socket, buf, strlen(buf), 0) < 0) {
    perror("L'envoi des informations UDP a échoué");
    return 1;
  }

  return 0;
}

void add_partie(int client_socket, int mode_jeu) {

  parties[index_partie].nb_joueurs = 0;
  parties[index_partie].partie_id = index_partie;
  parties[index_partie].mode_jeu = mode_jeu;
  add_player((index_partie * 4), mode_jeu, client_socket,
             index_partie); // fois 4 pour avoir des id uniques + on incremente nombre de joueurs dans fonction add_player

  parties[index_partie].port_udp = UDP_PORT + index_partie;
  snprintf(parties[index_partie].adresse_multicast,
           sizeof(parties[index_partie].adresse_multicast), "ff02::1:2:3:%x",
           index_partie + 1); // ff02, car on est en local
  parties[index_partie].socket_udp = init_multicast_socket();

  if (send_udp_info_to_client(0)) exit(EXIT_FAILURE);

  index_partie++;
}

void join_or_create(int client_socket, int mode_jeu) {
  for (int i = index_partie; i >= 0; i--) {
    if (parties[i].mode_jeu == mode_jeu && parties[i].nb_joueurs < 4) {
      int x = add_player((i * 4) + parties[i].nb_joueurs, mode_jeu,
                         client_socket, i);
      send_udp_info_to_client(x - 1);
      if (x == 4) send_multicast_message();

      return;
    }
  }

  add_partie(client_socket, mode_jeu);
}

void x_client_left(int x, int client_socket) {
  int res = 4 - x;
  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, SIZE_MSG, "%d", res);

  if (send(client_socket, buf, strlen(buf), 0) < 0) {
    perror("L'envoi du nombre de joueurs restants a échoué");
    exit(EXIT_FAILURE);
  }
}

void send_multicast_message() {
  int sock_udp = parties[index_partie].socket_udp;

  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  inet_pton(AF_INET6, parties[index_partie].adresse_multicast, &addr.sin6_addr);
  addr.sin6_port = htons(parties[index_partie].port_udp);

  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, SIZE_MSG, "La partie va commencer");

  if (sendto(sock_udp, buf, strlen(buf), 0, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
    perror("L'envoi du message multicast a échoué");
    exit(EXIT_FAILURE);
  }
}

void handle_client(int client_socket) {
  int mode_jeu = ask_game_mode(client_socket);

  int tmp = index_partie;
  join_or_create(client_socket, mode_jeu);
  if (tmp != index_partie)
    tmp = index_partie - 1;

  char buf[SIZE_MSG];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, SIZE_MSG, "Vous avez rejoint la partie, tu es joueur : %d", tmp);

  if (send(client_socket, buf, strlen(buf), 0) < 0) {
    perror("L'envoi du message de confirmation a échoué");
    exit(EXIT_FAILURE);
  }
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

  printf("Le joueur a choisi le mode de jeu : %s\n",
         (player_choice == 1 ? "multiplayer" : "singleplayer"));

  return player_choice;
}

int main() {
  int server_socket, client_socket;
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

  printf("Serveur démarré, en attente de connexions...\n");

  socklen_t addr_len = sizeof(client_addr);
  while (1) {
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket < 0) {
      perror("L'acceptation de la connexion a échoué");
      exit(EXIT_FAILURE);
    }

    handle_client(client_socket);
    close(client_socket);
  }

  close(server_socket);

  return 0;
}
