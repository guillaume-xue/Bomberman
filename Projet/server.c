#include "server.h"

#define MAX_PARTIES 4

Partie parties[MAX_PARTIES];
int index_partie = 0;

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

// Fonction pour envoyer les informations de la partie au joueur
void send_game_info(int client_socket, Partie* game) {
    ServerMessage server_message;
    server_message.code_req = game->mode_jeu;
    server_message.id = game->joueurs[game->nb_joueurs - 1].player_id;
    server_message.eq = (game->mode_jeu == 2) ? game->joueurs[game->nb_joueurs - 1].team_number : -1;
    server_message.port_udp = game->port_udp;
    server_message.port_m_diff = game->port_multicast; 
    //server_message.adr_m_diff = game->adresse_multicast; 
    strncpy(server_message.adr_m_diff, game->adresse_multicast, strlen(game->adresse_multicast));

    // Affichage des informations envoyées
    printf("Informations envoyées au client :\n");
    printf("Code de la requête : %d\n", server_message.code_req);
    printf("ID du joueur : %d\n", server_message.id);
    printf("Équipe du joueur : %d\n", server_message.eq);
    printf("Port UDP du serveur : %d\n", server_message.port_udp);
    printf("Port de multidiffusion du serveur : %d\n", server_message.port_m_diff);
    printf("Adresse de multidiffusion du serveur : %s\n", server_message.adr_m_diff);


    if (send(client_socket, &server_message, sizeof(ServerMessage), 0) < 0) {
        perror("L'envoi des informations de la partie a échoué");
        exit(EXIT_FAILURE);
    }
    printf("Les informations pour le jeu ont été envoyé aux clients \n");
}


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

  parties[index_partie].mode_jeu = (mode_jeu == 2) ? 10 : 9;
  
  add_player((index_partie * 4), mode_jeu, client_socket,
             index_partie); // fois 4 pour avoir des id uniques
  parties[index_partie].nb_joueurs = 1;
  parties[index_partie].port_udp = UDP_PORT + index_partie;

  snprintf(parties[index_partie].adresse_multicast,
           sizeof(parties[index_partie].adresse_multicast), "ff02::%x",
           index_partie + 1);

  parties[index_partie].port_multicast = MULTICAST_PORT + index_partie;

  index_partie++;

  // envoyer ces informations aux clients 
  send_game_info(client_socket,&(parties[index_partie]));
}

void join_or_create(int client_socket, int mode_jeu) {
  for (int i = index_partie; i >= 0; i--) {
    if (parties[i].mode_jeu == mode_jeu && parties[i].nb_joueurs < 4) {
      add_player((i * 4) + parties[i].nb_joueurs, mode_jeu, client_socket, i);
      return;
    }
  }
  add_partie(client_socket, mode_jeu);
}


void handle_client(int client_socket) {
    GameMessage received_message;
    // Réception du message depuis le client
    if (recv(client_socket, &received_message, sizeof(GameMessage), 0) < 0) {
        perror("La réception du message a échoué");
        exit(EXIT_FAILURE);
    }
    printf("Message reçu :\n");
    printf("CODEREQ : %d !!\n", received_message.CODEREQ);

    join_or_create(client_socket,received_message.CODEREQ);
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

    handle_client(client_socket);
    
    //close(client_socket);
  }

  close(server_socket);

  return 0;
}
