#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>


#define TCP_PORT 8080
#define MULTICAST_PORT 9000
#define SIZE_MSG 1024
#define INTERVALLE_ENVOI 1
#define TEXT_SIZE 255
#define NB_WALLS 100
#define MAX_CLIENTS 4
#define MAX_PARTIES 5

typedef enum ACTION { NONE, UP, DOWN, LEFT, RIGHT, BOMB, QUIT } ACTION;
enum colors { GREEN, YELLOW, MAGENTA, CYAN };

// Structure représentant une demande d'intégration à une partie
typedef struct {
    int CODEREQ; // Code de requête (1 ou 2 pour intégrer une partie, 3 ou 4 pour annoncer que le joueur est prêt à jouer)
    int ID; // Identifiant du joueur (0 à 3)
    int EQ; // Numéro de l'équipe du joueur (0 ou 1)
} RequeteIntegration;

// WARNING : Faites attention au passage de 8191 à 0.
typedef struct {
    int CODEREQ;  // 5: 1v3, 6: 2v2
    short ID; // ID du joueur
    int EQ;   // numéro de l'équipe du joueur si CODEREQ = 6
    int NUM;  // numéro du message modulo 2^13
    int ACTION; // 0: ↑, 1: →, 2: ↓, 3: ←, 4: ↗, 5: ↘, 6: bomb, 7: undo request
} GameMessage;

typedef struct {
    int code_req;
    short id;
    short eq;
    uint16_t port_udp;
    uint16_t port_m_diff;
    char adr_m_diff[INET6_ADDRSTRLEN];
} ServerMessage;

// Structure représentant les paramètres du serveur
typedef struct {
    int taille_grille; // Taille de la grille de jeu
    int temps_explosion_bombe; // Temps entre le dépôt d'une bombe et son explosion (en millisecondes)
    int freq_multidiff_grille; // Fréquence de multidiffusion de la grille complète (en millisecondes)
    int freq_traitement_requetes; // Fréquence de traitement des requêtes (en millisecondes)
} ParametresServeur;

typedef struct {
    int nb_joueurs;
    int partie_id;
    int mode_jeu;

    pthread_t thread_partie;  // thread pour gérer la partie, afin de pas bloquer le serveur

    int send_sock;  // sock pour envoyer des messages multicast aux clients
    struct sockaddr_in6 multicast_addr; // adresse multicast

    struct sockaddr_in6 partie_addr;  // adresse de la partie

    int clients_socket_tcp[MAX_CLIENTS];  // sockets des clients, pour communiquer avec eux
    int clients_playing[MAX_CLIENTS];  // id des clients, afin de savoir qui est encore connecté dans la partie
} Partie;

// Définition des valeurs pour le contenu de chaque case de la grille.
typedef enum {
    CASE_VIDE = 0,
    MUR_INDESTRUCTIBLE = 1,
    MUR_DESTRUCTIBLE = 2,
    BOMBE = 3,
    EXPLOSION = 4,
    JOUEUR0 = 5,
    JOUEUR1 = 6,
    JOUEUR2 = 7,
    JOUEUR3 = 8
} ContenuCase;

typedef struct {
    uint8_t CODEREQ;
    uint8_t ID;
    uint8_t EQ;
} EnteteMessage;

// Structure pour un message de mise à jour de cases spécifiques
typedef struct {
    EnteteMessage entete;
    u_int16_t NUM;    // Numéro du message modulo 2^16
    uint16_t NB;      // Nombre de cases mises à jour
    uint8_t **cases;  // Tableau de tableaux de uint8_t pour les cases de la grille
} GridMaj;


// Structure représentant la grille de jeu
typedef struct {
    EnteteMessage entete;
    u_int16_t NUM;    // Numéro du message modulo 2^16
    uint8_t longueur;
    uint8_t largeur;
    uint8_t** cases;  // Pointeur vers un tableau dynamique pour les cases de la grille
} GridData;

// Structure représentant TchatMessage
typedef struct {
    EnteteMessage entete;
    uint8_t LEN;
    char DATA[SIZE_MSG];
} TchatMessage;

typedef struct line {
    char data[TEXT_SIZE];
    int cursor;
} line;

typedef struct pos {
    int x;
    int y;
} pos;

typedef struct bomb {
    int x;
    int y;
    bool set;
} bomb;

typedef struct player {
    int id;
    pos *p;
    bomb *b;
    ACTION action;
} player;

#endif