#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

// Structure à envoyer
struct MyStruct {
    int id;
    float value;
};

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Création du socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Initialisation de la structure sockaddr_in
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Adresse IP du serveur
    server_addr.sin_port = htons(12345); // Port du serveur

    // Connexion au serveur
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erreur lors de la connexion au serveur");
        exit(EXIT_FAILURE);
    }

    // Création et envoi de la structure
    struct MyStruct data;
    data.id = 1;
    data.value = 3.14;

    // Sérialisation et envoi
    send(sockfd, &data, sizeof(data), 0);

    // Fermeture du socket
    close(sockfd);

    return 0;
}
