#include "grid_handler.h"

struct explose_arg {
    player p;
    GridData * g;
} explose_arg;

// Place les murs sur la grille
void setup_wall(GridData *g) {
    // On met des murs incassables sur les cases impaires
    for (int i = 1; i < g->largeur; i+=2) {
        for (int j = 1; j < g->hauteur; j+=2) {
            set_grid(g, i, j, 1);
        }
    }
    // On met des murs cassables aléatoirement
    int i = 0;
    while (i < NB_WALLS) {
        int x = rand() % g->largeur;
        int y = rand() % g->hauteur;
        if (get_grid(g, x, y) == 0){
            set_grid(g, x, y, 2);
            i++;
        }
    }
}

// Place les joueurs sur la grille
void setup_players(player * global_players, GridData * global_g) {
    for (int i = 0; i < 4; ++i) {
        set_grid(global_g, global_players[i].p->x, global_players[i].p->y, 5 + i);
    }
}

// Initialise la grille
void setup_grid(GridData *g, int hauteur, int largeur, player * p) {
    g->entete.CODEREQ = 11;
    g->NUM = 0; // Car premier message de la partie
    g->hauteur = hauteur - 2 - 1; // 2 rows reserved for border, 1 row for chat
    g->largeur = largeur - 2; // 2 columns reserved for border
    g->cases = calloc((g->largeur) * (g->hauteur), sizeof(int));
    if (g->cases == NULL) {
        perror("Memory allocation error for 'g->cases'");
        exit(EXIT_FAILURE);
    }
    init_player(p, g);
    setup_players(p, g);
    setup_wall(g);
}

// Initialise les joueurs
void init_player(player * global_players, GridData * global_g) {

    for (int i = 0; i < 4; i++) {
        global_players[i].id = i + 1;
        global_players[i].p = malloc(sizeof(pos));
        if (global_players[i].p == NULL) {
            perror("Memory allocation error for 'players[i]->p'");
            exit(EXIT_FAILURE);
        }
        global_players[i].b = malloc(sizeof(bomb));
        if (global_players[i].b == NULL) {
            perror("Memory allocation error for 'players[i]->b'");
            exit(EXIT_FAILURE);
        }
    }
    global_players[0].p->x = 0; global_players[0].p->y = 0;
    global_players[1].p->x = global_g->largeur - 1; global_players[1].p->y = 0;
    global_players[2].p->x = 0; global_players[2].p->y = global_g->hauteur - 1;
    global_players[3].p->x = global_g->largeur - 1; global_players[3].p->y = global_g->hauteur - 1;
}

// Fonction appelée par le thread pour faire exploser la bombe
void explose_handler(void * arg) {
    sleep(3);
    player global_players = ((struct explose_arg *)arg)->p;
    GridData * global_g = ((struct explose_arg *)arg)->g;
    explode_bomb(global_players, global_g);
}

// Fait exploser la bombe
void explode_bomb(player global_players, GridData * global_g){
    for (int i = global_players.b->x - 1; i <= global_players.b->x + 1; i++) {
        for (int j = global_players.b->y - 1; j <= global_players.b->y + 1; j++) {
            // On casse les murs cassables
            if (i >= 0 && i < global_g->largeur && j >= 0 && j < global_g->hauteur && is_wall_breakable(global_g,i, j)){
                clear_grid(global_g,i, j);

            }
        }
    }
    // On efface la bombe
    clear_grid(global_g, global_players.b->x, global_players.b->y);
    global_players.b->set = false;
}

bool perform_action(player p, GridData * g) {
    // Efface l'ancienne position du joueur
    clear_grid(g, p.p->x, p.p->y);

    int xd = 0;
    int yd = 0;

    // On met à jour l'action du joueur
    switch (p.action) {
        case LEFT:
            xd = -1; yd = 0; break;
        case RIGHT:
            xd = 1; yd = 0; break;
        case UP:
            xd = 0; yd = -1; break;
        case DOWN:
            xd = 0; yd = 1; break;
        case BOMB:
            p.b->set = true;
            pthread_t thread;
            explose_arg = (struct explose_arg){p, g};
            // Créer un thread pour exécuter fonctionA
            if (pthread_create(&thread, NULL, (void *(*)(void *)) explose_handler, (void *)&explose_arg) != 0){
                fprintf(stderr, "Erreur lors de la création du thread.\n");
                return 1;
            }
            break;
        case QUIT:
            return true;
        default: break;
    }

    // On met à jour la position de la bombe si elle est posée
    if (p.b->set) {
        set_grid(g, p.b->x, p.b->y, 3);
    }else{
        p.b->x = p.p->x;
        p.b->y = p.p->y;
    }

    // On vérifie si le joueur peut bouger
    if(is_movable(g, p.p->x + xd, p.p->y + yd)){

        // On met à jour la position du joueur
        p.p->x += xd;
        p.p->y += yd;

        // On bouge
        switch (p.id) {
            case 1:
                set_grid(g, p.p->x, p.p->y, 5);
                break;
            case 2:
                set_grid(g, p.p->x, p.p->y, 6);
                break;
            case 3:
                set_grid(g, p.p->x, p.p->y, 7);
                break;
            case 4:
                set_grid(g, p.p->x, p.p->y, 8);
                break;
            default:
                break;
        }
    }
    return false;
}

bool perform_action_all(player * p, GridData * g){
    for (int i = 0; i < 4; i++) {
        if (perform_action(p[i],g)) return true;
    }
    return false;
}

// Met à jour l'action des joueurs
void update_action(GameMessage *gmsg, player * p){
    ACTION a = NONE;
    switch (gmsg->ACTION) { // Update the action of the other players
        case 0:
            a = UP;
            break;
        case 1:
            a = RIGHT;
            break;
        case 2:
            a = DOWN;
            break;
        case 3:
            a = LEFT;
            break;
        case 4:
            a = BOMB;
            break;
        case 5:
            a = NONE;
        default:
            break;
    }
    p[gmsg->ID].action = a; // Update the action of the other players
}

bool is_movable(GridData *g, int x, int y) {
    return x >= 0 && x < g->largeur && y >= 0 && y < g->hauteur && !is_wall(g, x, y) && !is_bomb(g, x, y);
}

bool is_bomb(GridData *g, int x, int y) {
    return get_grid(g, x, y) == 3;
}

// Retourne vrai si la case est un mur cassable
bool is_wall_breakable(GridData *g, int x, int y){
    return get_grid(g, x, y) == 2;
}

// Retourne vrai si la case est un mur
bool is_wall(GridData *g, int x, int y) {
    return get_grid(g, x, y) == 1 || get_grid(g, x, y) == 2 ;
}

int get_grid(GridData *g, int x, int y) {
    return g->cases[y * g->largeur + x];
}

void set_grid(GridData *g, int x, int y, int v) {
    g->cases[y*g->largeur + x] = v;
}

void clear_grid(GridData *g, int x, int y) {
    set_grid(g, x, y, 0);
}

void free_player(player * p) {
    for (int i = 0; i < 4; i++) {
        free(p[i].p);
        free(p[i].b);
    }
    free(p);
}

void free_board(GridData * g) {
    free(g->cases);
}

void free_all(player * p, GridData * g) {
    free_player(p);
    free_board(g);
    free(g);
}

void start_game(GridData *g, player * p) {
    GameMessage *gmsg = malloc(sizeof(GameMessage));
    while (true){
        update_action(gmsg, p);
        if (perform_action_all(p, g)) break;
    }
    free_all(p, g);
}