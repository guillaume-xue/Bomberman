// Build with -lncurses option

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "data.h"

#define NB_WALLS 200

bomb* bombe;
board* b;

void setup_board(board* board) {
    int lines = 22; int columns = 51;
    board->hauteur = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
    board->largeur = columns - 2; // 2 columns reserved for border
    board->grid = calloc((board->largeur)*(board->hauteur),sizeof(char));
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

void free_board(board* board) {
    free(board->grid);
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
            switch (get_grid(b,x,y)) {
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

ACTION control(line* l) {
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
            if (bombe->set) l->data[(l->cursor)++] = '1';
            break;
    }
    return a;
}

void clear_grid(board* b, int x, int y) {
    set_grid(b, x, y, 0);
}

bool perform_action(board* b, pos* p, ACTION a) {
    // Efface l'ancienne position du joueur
    clear_grid(b, p->x, p->y);

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
            bombe->set = true;
            signal(SIGALRM, alarm_handler);
            alarm(3);
            break;
        case QUIT:
            return true;
        default: break;
    }

    if (bombe->set) {
        set_grid(b, bombe->x, bombe->y, 4);
        // Set the function to be called when SIGALRM signal is received
    }else{
        bombe->x = p->x;
        bombe->y = p->y;
    }

    if(is_movable(b, p->x + xd, p->y + yd)){
        // On bouge
        p->x += xd;
        p->y += yd;
        set_grid(b, p->x, p->y, 1);
    }

    return false;
}

bool is_movable(board* b, int x, int y) {
    return x >= 0 && x < b->largeur && y >= 0 && y < b->hauteur && !is_wall(b, x, y) && !is_bomb(b, x, y);
}

bool is_bomb(board* b, int x, int y) {
    return get_grid(b, x, y) == 4;
}

void explode_bomb(board* b){
    // On met les cases autour de la bombe à 0
    for (int i = bombe->x - 1; i <= bombe->x + 1; i++) {
        for (int j = bombe->y - 1; j <= bombe->y + 1; j++) {
            if (i >= 0 && i < b->largeur && j >= 0 && j < b->hauteur && is_wall_breakable(b, i, j)){
                clear_grid(b, i, j);
            }
        }
    }
    clear_grid(b, bombe->x, bombe->y);
    bombe->set = false;
    bombe->timer = 3000000000000;
}

void alarm_handler(int signum) {
    // This function will be called when the SIGALRM signal is received
    explode_bomb(b);
}

bool is_wall_breakable(board* b, int x, int y){
    return get_grid(b, x, y) == 3;
}

// Retourne vrai si la case est un mur
bool is_wall(board* b, int x, int y) {
    return get_grid(b, x, y) == 2 || get_grid(b, x, y) == 3 ;
}

int main()
{
    b = malloc(sizeof(board));;
    line* l = malloc(sizeof(line));
    l->cursor = 0;
    pos* p = malloc(sizeof(pos));
    p->x = 0; p->y = 0;
    bombe = malloc(sizeof(bomb));
    bombe->x = 0; bombe->y = 0; bombe->timer = 3000000000000; bombe->set = false;

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
        ACTION a = control(l);
        if (perform_action(b, p, a)) break;
        refresh_game(b,l);
        usleep(30*1000);
    }
    free_board(b);

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */

    free(p); free(l); free(b);
    free(bombe);

    return 0;
}
