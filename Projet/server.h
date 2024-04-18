#ifndef SERVER_H
#define SERVER_H

#include "config.h"

#include "func.h"

#define MAX_CLIENTS 4
#define MAX_PARTIES 4

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
    uint16_t NUM;
} EnteteMessage;

typedef struct {
    //EnteteMessage entete;
    uint8_t longueur;
    uint8_t largeur;
    uint8_t* cases;  // Pointeur vers un tableau dynamique pour les cases de la grille
} GridData;

// Structure pour un message de mise à jour de cases spécifiques
typedef struct {
    EnteteMessage entete;
    uint16_t NB;     // Nombre de cases mises à jour
    uint8_t* updates; // Pointeur vers un tableau dynamique pour les mises à jour
} GridMaj;

void init_mutex();
void add_player(Partie *partie, int client_socket);
void add_partie(int client_socket, int mode_jeu);
int join_or_create(int client_socket, int mode_jeu);
void *handle_client(void *arg);
void init_multicast_socket(Partie *partie);
void send_game_s_info(Partie *partie, int client_socket);

#endif
