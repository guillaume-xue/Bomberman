#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>

#define TCP_PORT 8080
#define UDP_PORT 9000
#define MAX_CLIENTS 4

typedef struct {
    int player_id;
    int team_number;
} Joueur;

typedef struct {
    Joueur joueurs[MAX_CLIENTS];
    char adresse_multicast[INET6_ADDRSTRLEN];
    int port_udp;
    int nb_joueurs;
} Partie;

void handle_client(int client_socket, Partie *parties, int *nb_parties) {
    char buffer[1024];
    int mode_jeu;
    recv(client_socket, &mode_jeu, sizeof(int), 0);

    // Création d'une nouvelle partie si elle n'existe pas
    if (parties[mode_jeu].nb_joueurs == 0) {
        parties[mode_jeu].port_udp = UDP_PORT + *nb_parties;
        inet_ntop(AF_INET6, &(in6addr_any), parties[mode_jeu].adresse_multicast, INET6_ADDRSTRLEN);
        printf("Nouvelle partie créée avec l'adresse de multidiffusion : %s et le port UDP : %d\n", parties[mode_jeu].adresse_multicast, parties[mode_jeu].port_udp);
        (*nb_parties)++;
    }

    // Attribution de l'identifiant du joueur et du numéro d'équipe
    int id_joueur = parties[mode_jeu].nb_joueurs;
    int numero_equipe = id_joueur % 2;
    parties[mode_jeu].joueurs[id_joueur].player_id = id_joueur;
    parties[mode_jeu].joueurs[id_joueur].team_number = numero_equipe;
    parties[mode_jeu].nb_joueurs++;

    // Envoi des données au client
    sprintf(buffer, "%d,%d,%s,%d", id_joueur, numero_equipe, parties[mode_jeu].adresse_multicast, parties[mode_jeu].port_udp);
    send(client_socket, buffer, sizeof(buffer), 0);
    printf("Le joueur %d a rejoint le mode de jeu %d\n", id_joueur, mode_jeu);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in6 server_addr, client_addr;
    int nb_parties = 0;
    Partie parties[2] = {0}; // On suppose qu'il y a deux modes de jeu

    // Création de la socket TCP
    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("La création de la socket a échoué");
        exit(EXIT_FAILURE);
    }

    // Initialisation de l'adresse du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(TCP_PORT);

    // Lier la socket au port spécifié
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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
    while (true) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("L'acceptation de la connexion a échoué");
            exit(EXIT_FAILURE);
        }

        // Traitement du client dans un nouveau thread
        handle_client(client_socket, parties, &nb_parties);
        close(client_socket);
    }

    close(server_socket);

    return 0;
}
