#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

// Structure à recevoir
struct MyStruct {
    int id;
    float value;
};

int main() {
    int sockfd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    
    // Création du socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }
    
    // Initialisation de la structure sockaddr_in
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Toute adresse IP
    server_addr.sin_port = htons(12345); // Port du serveur
    
    // Association de l'adresse et du port au socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erreur lors de l'association de l'adresse et du port au socket");
        exit(EXIT_FAILURE);
    }
    
    // Mise en écoute du socket
    if (listen(sockfd, 5) == -1) {
        perror("Erreur lors de la mise en écoute du socket");
        exit(EXIT_FAILURE);
    }
    
    // Acceptation de la connexion entrante
    client_len = sizeof(client_addr);
    new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (new_sockfd == -1) {
        perror("Erreur lors de l'acceptation de la connexion entrante");
        exit(EXIT_FAILURE);
    }
    
    // Réception des données
    struct MyStruct received_data;
    recv(new_sockfd, &received_data, sizeof(received_data), 0);
    
    // Affichage des données reçues
    printf("ID: %d\n", received_data.id);
    printf("Value: %f\n", received_data.value);
    
    // Fermeture des sockets
    close(new_sockfd);
    close(sockfd);
    
    return 0;
}
