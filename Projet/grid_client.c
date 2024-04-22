#include "grid_client.h"

int get_grid(GridData *g, int x, int y) {
    return g->cases[y * g->largeur + x];
}

void refresh_game(line *l, GridData *g) {
    int x, y;
    for (y = 0; y < g->hauteur; y++) {
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
    for (x = 0; x < g->largeur+2; x++) {
        mvaddch(0, x, '-');
        mvaddch(g->hauteur+1, x, '-');
    }
    for (y = 0; y < g->hauteur+2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, g->largeur+1, '|');
    }
    // Update chat text
    attron(COLOR_PAIR(1)); // Enable custom color 1
    attron(A_BOLD); // Enable bold
    for (x = 0; x < g->largeur+2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor)
            mvaddch(g->hauteur+2, x, ' ');
        else
            mvaddch(g->hauteur+2, x, l->data[x]);
    }
    attroff(A_BOLD); // Disable bold
    attroff(COLOR_PAIR(1)); // Disable custom color 1
    refresh(); // Apply the changes to the terminal
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

void print_grid(GridData grid, GameMessage gameMessage, line *tchat) {
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

    line *l = malloc(sizeof(line));
    if (l == NULL) {
        perror("Memory allocation error for 'l'");
        exit(EXIT_FAILURE);
    }
    l = tchat;

    GameMessage gmsg;
    memset(&gmsg, 0, sizeof(GameMessage));
    gmsg = gameMessage;

    while (true){
        ACTION a = control(l);
        gmsg.ACTION = a;
        if (a == QUIT) break;
        refresh_game(l, &g);
        usleep(30 * 1000);
    }

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */
}