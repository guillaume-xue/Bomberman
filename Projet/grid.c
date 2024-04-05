// Build with -lncurses option

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "grid.h"

void setup_board(board* b) {
    int lines = 22; int columns = 51;
    b->hauteur = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
    b->largeur = columns - 2; // 2 columns reserved for border
    b->grid = calloc((b->largeur) * (b->hauteur), sizeof(char));
}

// Place les murs sur la grille
void setup_wall(board* b) {
    // On met des murs incassables sur les cases impaires
    for (int i = 1; i < b->largeur; i+=2) {
        for (int j = 1; j < b->hauteur; j+=2) {
            set_grid(b, i, j, 2);
        }
    }
    // On met des murs cassables aléatoirement
    int i = 0;
    while (i < NB_WALLS) {
        int x = rand() % b->largeur;
        int y = rand() % b->hauteur;
        if (get_grid(b, x, y) != 1 && get_grid(b, x, y) != 2 && get_grid(b, x, y) != 3){
            set_grid(b, x, y, 3);
            i++;
        }
    }
}

void setup_player(board* b, player* p[4]) {
    // On initialise les bombes des joueurs
    for (int i = 0; i < 4; i++) {
        p[i]->bombe = malloc(sizeof(bomb));
        p[i]->bombe->x = 0;
        p[i]->bombe->y = 0;
        p[i]->bombe->set = false;
        set_grid(b, p[i]->position->x, p[i]->position->y, 1);
    }
    // On place les joueurs aux coins de la grille
    p[0]->position = malloc(sizeof(pos));
    p[0]->position->x = 0;
    p[0]->position->y = 0;
    p[1]->position = malloc(sizeof(pos));
    p[1]->position->x = b->largeur - 1;
    p[1]->position->y = 0;
    p[2]->position = malloc(sizeof(pos));
    p[2]->position->x = 0;
    p[2]->position->y = b->hauteur - 1;
    p[3]->position = malloc(sizeof(pos));
    p[3]->position->x = b->largeur - 1;
    p[3]->position->y = b->hauteur - 1;
}

void free_board(board* b) {
    free(b->grid);
}

int get_grid(board* b, int x, int y) {
    return b->grid[(y*b->largeur) + x];
}

void set_grid(board* b, int x, int y, int v) {
    b->grid[y*b->largeur + x] = v;
}

void refresh_game(board* b, line* l) {
    // Update grid
    int x,y;
    for (y = 0; y < b->hauteur; y++) {
        for (x = 0; x < b->largeur; x++) {
            char c;
            switch (get_grid(b, x, y)) {
                case 0:
                    c = ' ';
                    break;
                case 1:
                    c = 'O';
                    break;
                case 2:
                    c = 'U';
                    break;
                case 3:
                    c = 'B';
                    break;
                case 4:
                    c = 'X';
                    break;
                default:
                    c = '?';
                    break;
            }
            mvaddch(y+1,x+1,c);
        }
    }
    for (x = 0; x < b->largeur+2; x++) {
        mvaddch(0, x, '-');
        mvaddch(b->hauteur+1, x, '-');
    }
    for (y = 0; y < b->hauteur+2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, b->largeur+1, '|');
    }
    // Update chat text
    attron(COLOR_PAIR(1)); // Enable custom color 1
    attron(A_BOLD); // Enable bold
    for (x = 0; x < b->largeur+2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor)
            mvaddch(b->hauteur+2, x, ' ');
        else
            mvaddch(b->hauteur+2, x, l->data[x]);
    }
    attroff(A_BOLD); // Disable bold
    attroff(COLOR_PAIR(1)); // Disable custom color 1
    refresh(); // Apply the changes to the terminal
}

ACTION control(board* b, line* l) {
    int c;
    int prev_c = ERR;
    // We consume all similar consecutive key presses
    while ((c = getch()) != ERR) { // getch returns the first key press in the queue
        if (prev_c != ERR && prev_c != c) {
            ungetch(c); // put 'c' back in the queue
            break;
        }
        prev_c = c;
    }
    ACTION a = NONE;
    switch (prev_c) {
        case ERR: break;
        case KEY_LEFT:
            a = LEFT; break;
        case KEY_RIGHT:
            a = RIGHT; break;
        case KEY_UP:
            a = UP; break;
        case KEY_DOWN:
            a = DOWN; break;
        case 'b':
            a = BOMB; break;
        case '~':
            a = QUIT; break;
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

void clear_grid(board* b, int x, int y) {
    set_grid(b, x, y, 0);
}

bool perform_action(board* b, player *p, ACTION a) {
    // Efface l'ancienne position du joueur
    clear_grid(b, p->position->x, p->position->y);

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
            if (!p->bombe->set){
                explode_bomb(b, p);
                signal(SIGALRM, alarm_handler);
                alarm(3);
            }
            p->bombe->set = true;
            break;
        case QUIT:
            return true;
        default: break;
    }

    if (p->bombe->set) {
        set_grid(b, p->bombe->x, p->bombe->y, 4);
        // Set the function to be called when SIGALRM signal is received
    }else{
        p->bombe->x = p->position->x;
        p->bombe->y = p->position->y;
    }

    if(is_movable(b, p->position->x + xd, p->position->y + yd)){
        // On bouge
        p->position->x += xd;
        p->position->y += yd;
        set_grid(b, p->position->x, p->position->y, 1);
    }

    return false;
}

bool is_movable(board* b, int x, int y) {
    return x >= 0 && x < b->largeur && y >= 0 && y < b->hauteur && !is_wall(b, x, y) && !is_bomb(b, x, y);
}

bool is_bomb(board* b, int x, int y) {
    return get_grid(b, x, y) == 4;
}

void explode_bomb(board* b, player* p) {
    // On met les cases autour de la bombe à ' '
    for (int i = p->bombe->x - 1; i <= p->bombe->x + 1; i++) {
        for (int j = p->bombe->y - 1; j <= p->bombe->y + 1; j++) {
            if (i >= 0 && i < b->largeur && j >= 0 && j < b->hauteur && is_wall_breakable(b, i, j)){
                clear_grid(b, i, j);
            }
        }
    }
    clear_grid(b, p->bombe->x, p->bombe->y);
    p->bombe->set = false;
}

void alarm_handler(int signum) {
    // This function will be called when the SIGALRM signal is received
}

bool is_wall_breakable(board* b, int x, int y){
    return get_grid(b, x, y) == 3;
}

// Retourne vrai si la case est un mur
bool is_wall(board* b, int x, int y) {
    return get_grid(b, x, y) == 2 || get_grid(b, x, y) == 3 ;
}

int grid_creation()
{
    board* b = malloc(sizeof(board));;
    line* l = malloc(sizeof(line));
    l->cursor = 0;
    player *p[4];
    for (int i = 0; i < 4; ++i) {
        p[i] = malloc(sizeof(player));
    }
    setup_player(b, p);

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

    setup_board(b);
    setup_wall(b);

    while (true) {
        ACTION a = control(b, l);
        for (int i = 0; i < 4; ++i) {
            if (perform_action(b, p[i], a)) break;
        }
        refresh_game(b, l);
        usleep(30*1000);
    }

    free_board(b);

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */

    free(l); free(b);

    return 0;
}

int main() {
    grid_creation();
    return 0;
}