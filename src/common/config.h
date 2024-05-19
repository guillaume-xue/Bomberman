#ifndef CONFIG_H
#define CONFIG_H

#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <time.h>

#define TCP_PORT 8080
#define MULTICAST_PORT 9000
#define SIZE_MSG 1024
#define INTERVALLE_ENVOI 1
#define TEXT_SIZE 255
#define NB_WALLS 100
#define MAX_CLIENTS 4
#define MAX_PARTIES 5
#define MAX_CASES 50
#define FIELD_WIDTH 30  // la longueur réelle du terrain de jeu
#define FIELD_HEIGHT 15 // la hauteur réelle du terrain de jeu
#define TCHATBOX_HEIGHT 15 
#define TIMEOUT_SECONDS 10
#define DEBUG 1
typedef enum ACTION { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, BOMB = 4, QUIT = 5, NONE = 6, SUBMIT = 7 } ACTION;
enum colors { GREEN, YELLOW, MAGENTA, CYAN };

// Structure représentant une demande d'intégration à une partie
typedef struct {
  int CODEREQ; // Code de requête (1 ou 2 pour intégrer une partie, 3 ou 4 pour
               // annoncer que le joueur est prêt à jouer)
  int ID; // Identifiant du joueur (0 à 3)
  int EQ; // Numéro de l'équipe du joueur (0 ou 1)
} RequeteIntegration;

typedef struct pos {
  int x;
  int y;
} pos;

// WARNING : Faites attention au passage de 8191 à 0.
typedef struct {
  int CODEREQ; // 5: 1v3, 6: 2v2
  short ID;    // ID du joueur
  int EQ;      // numéro de l'équipe du joueur si CODEREQ = 6
  int NUM;     // numéro du message modulo 2^13
  int ACTION; // 0: ↑, 1: →, 2: ↓, 3: ←, 4: ↗, 5: ↘, 6: bomb, 7: undo request
} GameMessage;

// Définition des valeurs pour le contenu de chaque case de la grille.
typedef enum {
  CASE_VIDE = 0,
  MUR_INDESTRUCTIBLE = 1,
  MUR_DESTRUCTIBLE = 2,
  BOMBE = 3,
  EXPLOSION = 4,
  J0 = 5,
  J1 = 6,
  J2 = 7,
  J3 = 8
} ContenuCase;

typedef struct {
  uint8_t CODEREQ;
  uint8_t ID;
  uint8_t EQ;
} EnteteMessage;

// Structure représentant la grille de jeu
typedef struct {
  EnteteMessage entete;
  u_int16_t NUM; // Numéro du message modulo 2^16
  uint8_t width;
  uint8_t height;
  uint8_t cases[MAX_CASES][MAX_CASES];
} GridData;

typedef struct {
  EnteteMessage entete;
  uint16_t NUM;
  uint8_t NB;
  uint8_t DATA[MAX_CASES];
} FreqMessage;

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
  int taille_grille;            // Taille de la grille de jeu
  int temps_explosion_bombe;    // Temps entre le dépôt d'une bombe et son
                                // explosion (en millisecondes)
  int freq_multidiff_grille;    // Fréquence de multidiffusion de la grille
                                // complète (en millisecondes)
  int freq_traitement_requetes; // Fréquence de traitement des requêtes (en
                                // millisecondes)
} ParametresServeur;

typedef struct bomb {
    int x;
    int y;
    bool set;
} bomb;

typedef struct player {
    int id;
    pos p;
    bomb b;
    bool dead;
} player;

typedef struct {
  int nb_joueurs;
  int partie_id;
  int mode_jeu;

  GridData grid;
  player players[MAX_CLIENTS];

  int send_sock; // sock pour envoyer des messages multicast aux clients
  struct sockaddr_in6 multicast_addr; // adresse multicast

  struct sockaddr_in6 partie_addr; // adresse de la partie

  int clients_socket_tcp[MAX_CLIENTS]; // sockets des clients, pour communiquer
                                       // avec eux
  int clients_playing[MAX_CLIENTS];    // id des clients, afin de savoir qui est
                                       // encore connecté dans la partie
} Partie;

// Structure représentant TchatMessage
typedef struct {
  EnteteMessage entete;
  uint8_t LEN;
  char DATA[SIZE_MSG];
} TchatMessage;

typedef struct data {
  int index_partie;
  int id;
} data;

typedef struct {
    char conv[TCHATBOX_HEIGHT - 3][TEXT_SIZE]; // Tableau de caractères pour la conversation
    int i;                // Indice pour suivre la conversation
    int nb_lines;         // Nombre de lignes dans la conversation
} tchatbox;

typedef struct {
    char data[TEXT_SIZE]; // Tableau de caractères pour la ligne de texte
    int cursor;           // Indice pour le curseur
    tchatbox tchatbox;
} line;

#endif