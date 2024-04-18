#include "grid_handler.h"

player **global_players;
board *global_b;
int bombe;

void setup_board() { // Initialize the board
    int lines = 22;
    int columns = 51;
    global_b = malloc(sizeof(board));
    if (global_b == NULL) {
        perror("Memory allocation error for 'b'");
        exit(EXIT_FAILURE);
    }
    global_b->HAUTEUR = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
    global_b->LARGEUR = columns - 2; // 2 columns reserved for border
    global_b->cases = calloc((global_b->LARGEUR) * (global_b->HAUTEUR), sizeof(char));
    if (global_b->cases == NULL) {
        perror("Memory allocation error for 'b->grid'");
        exit(EXIT_FAILURE);
    }
}

// Place les murs sur la grille
void setup_wall() {
    // On met des murs incassables sur les cases impaires
    for (int i = 1; i < global_b->LARGEUR; i+=2) {
        for (int j = 1; j < global_b->HAUTEUR; j+=2) {
            set_grid(i, j, 1);
        }
    }
    // On met des murs cassables aléatoirement
    int i = 0;
    while (i < NB_WALLS) {
        int x = rand() % global_b->LARGEUR;
        int y = rand() % global_b->HAUTEUR;
        if (get_grid(x, y) == 0){
            set_grid(x, y, 2);
            i++;
        }
    }
}

// Place les joueurs sur la grille
void setup_players_pos() {
    global_players[0]->p->x = 0; global_players[0]->p->y = 0;
    global_players[1]->p->x = global_b->LARGEUR - 1; global_players[1]->p->y = 0;
    global_players[2]->p->x = 0; global_players[2]->p->y = global_b->HAUTEUR - 1;
    global_players[3]->p->x = global_b->LARGEUR - 1; global_players[3]->p->y = global_b->HAUTEUR - 1;
}

// Initialise les joueurs
void init_player() {
    global_players = malloc(4 * sizeof(player));
    for (int i = 0; i < 4; i++) {
        global_players[i] = malloc(sizeof(player));
        if (global_players[i] == NULL) {
            perror("Memory allocation error for 'players[i]'");
            exit(EXIT_FAILURE);
        }
        global_players[i]->p = malloc(sizeof(pos));
        if (global_players[i]->p == NULL) {
            perror("Memory allocation error for 'players[i]->p'");
            exit(EXIT_FAILURE);
        }
        global_players[i]->b = malloc(sizeof(bomb));
        if (global_players[i]->b == NULL) {
            perror("Memory allocation error for 'players[i]->b'");
            exit(EXIT_FAILURE);
        }
    }
    setup_players_pos();
}

// Fait exploser la bombe
void explode_bomb(){
    for (int i = global_players[bombe]->b->x - 1; i <= global_players[bombe]->b->x + 1; i++) {
        for (int j = global_players[bombe]->b->y - 1; j <= global_players[bombe]->b->y + 1; j++) {
            // On casse les murs cassables
            if (i >= 0 && i < global_b->LARGEUR && j >= 0 && j < global_b->HAUTEUR && is_wall_breakable(i, j)){
                clear_grid(i, j);
            }
        }
    }
    // On efface la bombe
    clear_grid(global_players[bombe]->b->x, global_players[bombe]->b->y);
    global_players[bombe]->b->set = false;
}

void alarm_handler(int signum) {
    // This function will be called when the SIGALRM signal is received
    explode_bomb();
}

bool perform_action(player * p) {
    // Efface l'ancienne position du joueur
    clear_grid(p->p->x, p->p->y);

    int xd = 0;
    int yd = 0;

    // On met à jour l'action du joueur
    switch (p->action) {
        case LEFT:
            xd = -1; yd = 0; break;
        case RIGHT:
            xd = 1; yd = 0; break;
        case UP:
            xd = 0; yd = -1; break;
        case DOWN:
            xd = 0; yd = 1; break;
        case BOMB:
            // On devrait mettre un mutex
            p->b->set = true;
            bombe = p->id-1;
            signal(SIGALRM, alarm_handler);
            alarm(3);
            break;
        case QUIT:
            return true;
        default: break;
    }

    // On met à jour la position du joueur
    p->p->x += xd;
    p->p->y += yd;

    // On met à jour la position de la bombe si elle est posée
    if (p->b->set) {
        set_grid(p->b->x, p->b->y, 3);
    }else{
        p->b->x = p->p->x;
        p->b->y = p->p->y;
    }

    // On vérifie si le joueur peut bouger
    if(is_movable(p->p->x + xd, p->p->y + yd)){
        // On bouge
        switch (p->id) {
            case 1:
                set_grid(p->p->x, p->p->y, 5);
                break;
            case 2:
                set_grid(p->p->x, p->p->y, 6);
                break;
            case 3:
                set_grid(p->p->x, p->p->y, 7);
                break;
            case 4:
                set_grid(p->p->x, p->p->y, 8);
                break;
            default:
                break;
        }
    }
    return false;
}

bool perform_action_all(){
    for (int i = 0; i < 4; i++) {
        if (perform_action(global_players[i])) return true;
    }
    return false;
}

// Met à jour l'action des joueurs
void update_action(GameMessage *gmsg){
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
    global_players[gmsg->ID]->action = a; // Update the action of the other players
}

bool is_movable(int x, int y) {
    return x >= 0 && x < global_b->LARGEUR && y >= 0 && y < global_b->HAUTEUR && !is_wall(x, y) && !is_bomb(x, y);
}

bool is_bomb(int x, int y) {
    return get_grid(x, y) == 3;
}

// Retourne vrai si la case est un mur cassable
bool is_wall_breakable(int x, int y){
    return get_grid(x, y) == 2;
}

// Retourne vrai si la case est un mur
bool is_wall(int x, int y) {
    return get_grid(x, y) == 1 || get_grid(x, y) == 2 ;
}

int get_grid(int x, int y) {
    return global_b->cases[(y * global_b->LARGEUR) + x];
}

void set_grid(int x, int y, int v) {
    global_b->cases[y * global_b->LARGEUR + x] = v;
}

void clear_grid(int x, int y) {
    set_grid(x, y, 0);
}

void free_player() {
    for (int i = 0; i < 4; i++) {
        free(global_players[i]->p);
        free(global_players[i]->b);
        free(global_players[i]);
    }
    free(global_players);
}

void free_board() {
    free(global_b->cases);
}

void free_all() {
    free_player();
    free_board();
    free(global_b);
}

int grid_creation(GameMessage *gmsg) {

    // Initialisation de la ligne de chat
    line* l = malloc(sizeof(line) + 1);
    if (l == NULL) {
        perror("Memory allocation error for 'l'");
        exit(EXIT_FAILURE);
    }
    l->cursor = 0;

    // Initialize the grid
    init_player();
    setup_board();
    setup_wall();

    while (true) {
        update_action(gmsg);
        if (perform_action_all()) break;
        usleep(30 * 1000);
    }

    free_all();

    return 0;
}