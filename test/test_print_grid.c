#include "../Projet/config.h"
#include <ncurses.h>

struct explose_arg {
    player p;
    GridData * g;
} explose_arg;

int get_grid(GridData *g, int x, int y) {
    return g->cases[x][y];
}

void set_grid(GridData *g, int x, int y, int v) {
    g->cases[x][y] = v;
}

void clear_grid(GridData *g, int x, int y) {
    set_grid(g, x, y, 0);
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

bool is_movable(GridData *g, int x, int y) {
    return x >= 0 && x < g->width && y >= 0 && y < g->height && !is_wall(g, x, y) && !is_bomb(g, x, y);
}

ACTION control(line* l) {
    int c;
    int prev_c = ERR;
    while ((c = getch()) != ERR) {
        if (prev_c != ERR && prev_c != c) {
            ungetch(c);
            break;
        }
        prev_c = c;
    }
    ACTION a = NONE;
    switch (prev_c) {
        case ERR: break;
        case KEY_LEFT: a = LEFT; break;
        case KEY_RIGHT: a = RIGHT; break;
        case KEY_UP: a = UP; break;
        case KEY_DOWN: a = DOWN; break;
        case 'b': a = BOMB; break;
        case '~': a = QUIT; break;
        case KEY_BACKSPACE:
            if (l->cursor > 0) l->cursor--;
            break;
        default:
            if (prev_c >= ' ' && prev_c <= '~' && l->cursor < TEXT_SIZE)
                l->data[(l->cursor)++] = prev_c;
            break;
    }
    return a;
}

// Fait exploser la bombe
void explode_bomb(player global_players, GridData * global_g){
    for (int i = global_players.b->x - 1; i <= global_players.b->x + 1; i++) {
        for (int j = global_players.b->y - 1; j <= global_players.b->y + 1; j++) {
            // On casse les murs cassables
            if (i >= 0 && i < global_g->width && j >= 0 && j < global_g->height && is_wall_breakable(global_g,i, j)){
                clear_grid(global_g,i, j);

            }
        }
    }
    // On efface la bombe
    clear_grid(global_g, global_players.b->x, global_players.b->y);
    global_players.b->set = false;
}

// Fonction appelée par le thread pour faire exploser la bombe
void explose_handler(void * arg) {
    sleep(3);
    player global_players = ((struct explose_arg *)arg)->p;
    GridData * global_g = ((struct explose_arg *)arg)->g;
    explode_bomb(global_players, global_g);
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

void refresh_game(line *l, GridData *g) {
    int x, y;
    for (y = 0; y < g->height; y++) {
        for (x = 0; x < g->width; x++) {
            char c;
            switch (get_grid(g, x, y)) {
                case 0: c = ' '; break;
                case 1: c = 'U'; break;
                case 2: c = 'B'; break;
                case 3: c = 'X'; break;
                case 4: c = 'Y'; break;
                case 5: c = '1'; break;
                case 6: c = '2'; break;
                case 7: c = '3'; break;
                case 8: c = '4'; break;
                default: c = '?'; break;
            }
            mvaddch(y + 1, x + 1, c);
        }
    }
    for (x = 0; x < g->width+2; x++) {
        mvaddch(0, x, '-');
        mvaddch(g->height+1, x, '-');
    }
    for (y = 0; y < g->height+2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, g->width+1, '|');
    }
    // Update chat text
    attron(COLOR_PAIR(1)); // Enable custom color 1
    attron(A_BOLD); // Enable bold
    for (x = 0; x < g->width+2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor)
            mvaddch(g->height+2, x, ' ');
        else
            mvaddch(g->height+2, x, l->data[x]);
    }
    attroff(A_BOLD); // Disable bold
    attroff(COLOR_PAIR(1)); // Disable custom color 1
    refresh(); // Apply the changes to the terminal
}

// Place les murs sur la grille
void setup_wall(GridData *g) {
    // On met des murs incassables sur les cases impaires
    for (int i = 1; i < g->width; i+=2) {
        for (int j = 1; j < g->height; j+=2) {
            set_grid(g, i, j, 1);
        }
    }
    // On met des murs cassables aléatoirement
    int i = 0;
    while (i < NB_WALLS) {
        int x = rand() % g->width;
        int y = rand() % g->height;
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
    global_players[1].p->x = global_g->width - 1; global_players[1].p->y = 0;
    global_players[2].p->x = 0; global_players[2].p->y = global_g->height - 1;
    global_players[3].p->x = global_g->width - 1; global_players[3].p->y = global_g->height - 1;
}

// Initialise la grille
void setup_grid(GridData *g, int hauteur, int largeur, player * p) {
    g->entete.CODEREQ = 11;
    g->NUM = 0; // Car premier message de la partie
    g->height = hauteur - 2 - 1; // 2 rows reserved for border, 1 row for chat
    g->width = largeur - 2; // 2 columns reserved for border
    init_player(p, g);
    setup_players(p, g);
    setup_wall(g);
}

void print_grid(GridData grid, player * players, GameMessage gameMessage, line tchat) {
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

    GridData g;
    memset(&g, 0, sizeof(GridData) - 1);
    g = grid;

    GameMessage gmsg;
    memset(&gmsg, 0, sizeof(GameMessage));
    gmsg = gameMessage;

    player *p = malloc(4 * sizeof(player *));
    if (p == NULL) {
        perror("Memory allocation error for 'p'");
        exit(EXIT_FAILURE);
    }
    p = players;
    setup_grid(&g, 20, 51, p);

    line l;
    memset(&l, 0, sizeof(line) - 1);
    l = tchat;

    while (true){
        ACTION a = control(&l);
        gmsg.ACTION = a;
        p[gmsg.ID].action = gmsg.ACTION;
        // envoi de l'action
        // envoi du tchat
        if(perform_action_all(p, &g)) break;
        // reception du tchat
        // reception de la grille
        refresh_game(&l, &g);
        usleep(30 * 1000);
    }

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */
}

int main(){

    GridData g;
    memset(&g, 0, sizeof(GridData) - 1);

    player *p = malloc(4 * sizeof(player *));
    if (p == NULL) {
        perror("Memory allocation error for 'p'");
        exit(EXIT_FAILURE);
    }

    setup_grid(&g, 20, 51, p);

    line l;
    memset(&l, 0, sizeof(line) - 1);
    l.cursor = 0;

    GameMessage me;
    memset(&me, 0, sizeof(GameMessage));
    me.CODEREQ = 2;
    me.ID = 1;
    me.EQ = 1;

    print_grid(g, p,me, l);
    return 0;
}