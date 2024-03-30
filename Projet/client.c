#include "client.h"

// Fonction pour envoyer une demande de jeu au serveur
void send_game_request(int client_socket, char* CODEREQ) {
     GameMessage request;
    memset(&request, 0, sizeof(GameMessage));
    
    snprintf(request.CODEREQ, sizeof(request.CODEREQ), "%s", CODEREQ);

    printf("CODEREQ : %s \n", request.CODEREQ);
    if (send(client_socket, &request, sizeof(GameMessage), 0) < 0) {
        perror("L'envoi de la demande de jeu a échoué");
        exit(EXIT_FAILURE);
    }
    printf(" Message envoyé \n"); 
}


// Fonction pour envoyer un message de chat au serveur
void send_chat_message(int client_socket, char* CODEREQ, short ID, char EQ, char *message) {
     GameMessage message_packet;
    strncpy(message_packet.CODEREQ, CODEREQ, sizeof(message_packet.CODEREQ));
    message_packet.ID = htons(ID);
    message_packet.EQ = EQ;

    // Envoi de la longueur du message
    size_t message_length = strlen(message);
    if (send(client_socket, &message_length, sizeof(message_length), 0) < 0) {
        perror("L'envoi de la longueur du message a échoué");
        exit(EXIT_FAILURE);
    }

    // Envoi du message
    if (send(client_socket, message, message_length, 0) < 0) {
        perror("L'envoi du message de chat a échoué");
        exit(EXIT_FAILURE);
    }
}

// Fonction pour recevoir les messages du serveur
void receive_message(int client_socket, void *buffer, size_t buffer_size) {
    if (recv(client_socket, buffer, buffer_size, 0) < 0) {
        perror("La réception du message a échoué");
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
      //char* mode = "2";
      send_game_request(client_socket, "Notre");
  } 

  return 0;
}