#include "grid_handler.h"

board *b;
player **players;

void setup_board() { // Initialize the board
    int lines = 22;
    int columns = 51;
    b = malloc(sizeof(board)); // Allocate memory for 'b'
    if (b == NULL) {
        perror("Memory allocation error for 'b'");
        exit(EXIT_FAILURE);
    }
    b->hauteur = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
    b->largeur = columns - 2; // 2 columns reserved for border
    b->grid = calloc((b->largeur) * (b->hauteur), sizeof(char));
    if (b->grid == NULL) {
        perror("Memory allocation error for 'b->grid'");
        exit(EXIT_FAILURE);
    }
}

// Place les murs sur la grille
void setup_wall() {
    // On met des murs incassables sur les cases impaires
    for (int i = 1; i < b->largeur; i+=2) {
        for (int j = 1; j < b->hauteur; j+=2) {
            set_grid(i, j, 1);
        }
    }
    // On met des murs cassables aléatoirement
    int i = 0;
    while (i < NB_WALLS) {
        int x = rand() % b->largeur;
        int y = rand() % b->hauteur;
        if (get_grid(x, y) == 0){
            set_grid(x, y, 2);
            i++;
        }
    }
}

// Place les joueurs sur la grille
void setup_players_pos() {
    players[0]->p->x = 0; players[0]->p->y = 0;
    players[1]->p->x = b->largeur - 1; players[1]->p->y = 0;
    players[2]->p->x = 0; players[2]->p->y = b->hauteur-1;
    players[3]->p->x = b->largeur - 1; players[3]->p->y = b->hauteur-1;
}

void free_board() {
    free(b->grid);
}

int get_grid(int x, int y) {
    return b->grid[(y*b->largeur) + x];
}

void set_grid(int x, int y, int v) {
    b->grid[y*b->largeur + x] = v;
}

void clear_grid(int x, int y) {
    set_grid(x, y, 0);
}

bool perform_action(player * p, ACTION a) {
    // Efface l'ancienne position du joueur
    clear_grid(p->p->x, p->p->y);

    int xd = 0;
    int yd = 0;

    switch (a) {
        case LEFT:
            xd = -1; yd = 0; break;
        case RIGHT:
            xd = 1; yd = 0; break;
        case UP:
            xd = 0; yd = -1; break;
        case DOWN:
            xd = 0; yd = 1; break;
        case BOMB:
            p->b->set = true;
            signal(SIGALRM, alarm_handler);
            alarm(3);
            break;
        case QUIT:
            return true;
        default: break;
    }

    if (p->b->set) {
        set_grid(p->b->x, p->b->y, 3);
    }else{
        p->b->x = p->p->x;
        p->b->y = p->p->y;
    }

    if(is_movable(p->p->x + xd, p->p->y + yd)){
        // On bouge
        p->p->x += xd;
        p->p->y += yd;
        switch (p->gmsg->ID) {
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
        if (perform_action(players[i], players[i]->action)) return true;
    }
    return false;
}

bool is_movable(int x, int y) {
    return x >= 0 && x < b->largeur && y >= 0 && y < b->hauteur && !is_wall(x, y) && !is_bomb(x, y);
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

void alarm_handler(int signum) {
    // This function will be called when the SIGALRM signal is received
    explode_bomb();
}

// Fait exploser la bombe
void explode_bomb(){
    for (int i = players[1]->b->x - 1; i <= players[1]->b->x + 1; i++) {
        for (int j = players[1]->b->y - 1; j <= players[1]->b->y + 1; j++) {
            // On casse les murs cassables
            if (i >= 0 && i < b->largeur && j >= 0 && j < b->hauteur && is_wall_breakable(i, j)){
                clear_grid(i, j);
            }
        }
    }
    // On efface la bombe
    clear_grid(players[1]->b->x, players[1]->b->y);
    players[1]->b->set = false;
}

// Met à jour l'action des joueurs
void update_action(line *l){
    for (int i = 0; i < 4; ++i) {
        ACTION a = NONE;
        switch (players[i]->gmsg->ACTION) { // Update the action of the other players
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
        players[i]->action = a; // Update the action of the other players
    }
}

void init_player() {
    for (int i = 0; i < 4; i++) {
        players[i] = malloc(sizeof(player));
        if (players[i] == NULL) {
            perror("Memory allocation error for 'players[i]'");
            exit(EXIT_FAILURE);
        }
        players[i]->p = malloc(sizeof(pos));
        if (players[i]->p == NULL) {
            perror("Memory allocation error for 'players[i]->p'");
            exit(EXIT_FAILURE);
        }
        players[i]->b = malloc(sizeof(bomb));
        if (players[i]->b == NULL) {
            perror("Memory allocation error for 'players[i]->b'");
            exit(EXIT_FAILURE);
        }
        players[i]->gmsg = malloc(sizeof(GameMessage));
        if (players[i]->gmsg == NULL) {
            perror("Memory allocation error for 'players[i]->gmsg'");
            exit(EXIT_FAILURE);
        }
    }
}

void set_gameMassage(GameMessage *gmsg, int ID, int ACTION, int CODEREQ, int EQ, int NUM) {
    gmsg->CODEREQ = 0;
    gmsg->ID = ID;
    gmsg->EQ = 0;
    gmsg->NUM = 0;
    gmsg->ACTION = ACTION;
}

void free_player() {
    for (int i = 0; i < 4; i++) {
        free(players[i]->p);
        free(players[i]->b);
        free(players[i]->gmsg);
        free(players[i]);
    }
    free(players);
}

int grid_creation() {
    players = malloc(4 * sizeof(player*));
    if (players == NULL) {
        perror("Memory allocation error for 'players'");
        exit(EXIT_FAILURE);
    }
    init_player();

    line* l = malloc(sizeof(line) + 1);
    if (l == NULL) {
        perror("Memory allocation error for 'l'");
        exit(EXIT_FAILURE);
    }
    l->cursor = 0;

    // NOTE: All ncurses operations (getch, mvaddch, refresh, etc.) must be done on the same thread.
    initscr(); /* Start curses mode */
    raw(); /* Disable line buffering */
    intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE); /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE); /* Make getch non-blocking */
    noecho(); /* Don't echo() while we do getch (we will manually print characters when relevant) */
    curs_set(0); // Set the cursor to invisible
    start_color(); // Enable colors
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // Define a new color style (text is yellow, background is black)

    setup_board();
    setup_players_pos();
    setup_wall();

    while (true) {
        update_action(l);
        if (perform_action_all()) break;
        usleep(30 * 1000);
    }

    free_board();

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */

    free(l);
    free(b);
    free_player();

    return 0;
}