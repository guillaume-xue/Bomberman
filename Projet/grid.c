#include <ncurses.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "grid.h"

int get_grid(GridData *g, int x, int y) {
    return g->cases[y][x];
}

void refresh_game(line *l, GridData *g) {
    int x, y;
    for (y = 0; y < g->longueur; y++) {
        for (x = 0; x < g->largeur; x++) {
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
    for (x = 0; x < g->largeur + 2; x++) {
        mvaddch(0, x, '-');
        mvaddch(g->longueur + 1, x, '-');
    }
    for (y = 0; y < g->longueur + 2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, g->largeur + 1, '|');
    }
    attron(COLOR_PAIR(1));
    attron(A_BOLD);
    for (x = 0; x < g->largeur + 2; x++) {
        mvaddch(g->longueur + 2, x, (x >= l->cursor) ? ' ' : l->data[x]);
    }
    attroff(A_BOLD);
    attroff(COLOR_PAIR(1));
    refresh();
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

bool perform_action(player *p) {
    int xd = 0, yd = 0;
    switch (p->action) {
        case LEFT: xd = -1; yd = 0; break;
        case RIGHT: xd = 1; yd = 0; break;
        case UP: xd = 0; yd = -1; break;
        case DOWN: xd = 0; yd = 1; break;
        case BOMB: p->b->set = true; break;
        case QUIT: return true;
        default: break;
    }
    p->p->x += xd;
    p->p->y += yd;
    p->b->x = p->p->x;
    p->b->y = p->p->y;
    return false;
}

void print_grid(player *current_player, GridData *g) {
    line* l = malloc(sizeof(line));
    if (!l) {
        perror("Memory allocation error for 'l'");
        exit(EXIT_FAILURE);
    }
    l->cursor = 0;

    initscr();
    raw();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    noecho();
    curs_set(0);
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);

    while (true) {
        current_player->action = control(l);
        if (perform_action(current_player)) break;
        refresh_game(l, g);
        usleep(30 * 1000);  // Sleep for 30 milliseconds
    }

    curs_set(1);
    endwin();
    free(l);
}
