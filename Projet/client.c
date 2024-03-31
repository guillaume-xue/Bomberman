#include "client.h"

void subscribe_multicast(int client_socket, uint16_t port_m_diff, char* adresse_m_diff) {
    struct sockaddr_in6 multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin6_family = AF_INET6;
    multicast_addr.sin6_port = htons(port_m_diff);
    inet_pton(AF_INET6, adresse_m_diff, &multicast_addr.sin6_addr);

    if (connect(client_socket, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("La connexion à l'adresse de multidiffusion a échoué");
        exit(EXIT_FAILURE);
    }
}

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
    printf(" Message envoyé \n"); 

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
        printf("Port UDP : %d\n", mess_serv.port_udp);
        printf("Port de multidiffusion : %d\n", mess_serv.port_m_diff);
        printf("Adresse de multidiffusion : %s\n", mess_serv.adr_m_diff);
    }

    printf("Fin\n");
}

void send_ready_message(int client_socket) {
    char ready_message[] = "READY";
    if (send(client_socket, ready_message, sizeof(ready_message), 0) < 0) {
        perror("L'envoi du message 'READY' a échoué");
        exit(EXIT_FAILURE);
    }
}


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
  // Demander si on est en mode equipe (cdreq 2 ) ou contre ( cdreq 1)
  puts(" En équipe ou non: 1 or 2\n1. Seul\n2. Équipe\n");

  char buf[20];
  while (1) {
    scanf("%s", buf);

    if (atoi(buf) == 1 || atoi(buf) == 2) {
      game_mode = atoi(buf);
      break;
    } else {
      puts("Mode invalide. 1 (Seul) or 2 "
           "(Équipe)");
    }
  }

  if (send(client_socket, &game_mode, sizeof(game_mode), 0) < 0) {
    perror("L'envoi du mode de jeu a échoué");
    exit(EXIT_FAILURE);
  }

  // Lancement du mode de jeu après le choix 
  if(game_mode == 1){
      grid_creation();
  }
  else{
      send_game_request(client_socket, 2);

  } 

  return 0;
}