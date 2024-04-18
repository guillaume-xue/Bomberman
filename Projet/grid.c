// Build with -lncurses option

#include <ncurses.h>
#include <signal.h>
#include "grid.h"

int get_grid(board *b, int x, int y) {
    return b->cases[(y * b->LARGEUR) + x];
}

void refresh_game(line *l, board *b) {
    // Update grid
    int x,y;
    for (y = 0; y < b->HAUTEUR; y++) {
        for (x = 0; x < b->LARGEUR; x++) {
            char c;
            switch (get_grid(b, x, y)) {
                case 0:
                    c = ' '; // Empty
                    break;
                case 1:
                    c = 'U'; // Wall unbreakable
                    break;
                case 2:
                    c = 'B'; // Wall breakable
                    break;
                case 3:
                    c = 'X'; // Bomb
                    break;
                case 4:
                    c = 'Y'; // Explosion
                    break;
                case 5:
                    c = '1'; // Player 1
                    break;
                case 6:
                    c = '2'; // Player 2
                    break;
                case 7:
                    c = '3'; // Player 3
                    break;
                case 8:
                    c = '4'; // Player 4
                    break;
                default:
                    c = '?';
                    break;
            }
            mvaddch(y+1,x+1,c);
        }
    }
    for (x = 0; x < b->LARGEUR + 2; x++) {
        mvaddch(0, x, '-');
        mvaddch(b->HAUTEUR + 1, x, '-');
    }
    for (y = 0; y < b->HAUTEUR + 2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, b->LARGEUR + 1, '|');
    }
    // Update chat text
    attron(COLOR_PAIR(1)); // Enable custom color 1
    attron(A_BOLD); // Enable bold
    for (x = 0; x < b->LARGEUR + 2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor)
            mvaddch(b->HAUTEUR + 2, x, ' ');
        else
            mvaddch(b->HAUTEUR + 2, x, l->data[x]);
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
            break;
    }
    return a;
}

bool perform_action(player *p) {
    int xd = 0;
    int yd = 0;
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
            p->b->set = true;
            break;
        case QUIT:
            return true;
        default: break;
    }

    p->p->x += xd;
    p->p->y += yd;
    p->b->x = p->p->x;
    p->b->y = p->p->y;

    return false;
}

void print_grid(player *current_player, board *b) {

    line* l = malloc(sizeof(line));
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

    while (true) {
        current_player->action = control(l); // Update the action of the current player
        if (perform_action(current_player)) break;
        refresh_game(l, b);
        usleep(30 * 1000);
    }

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */

    free(l);
}