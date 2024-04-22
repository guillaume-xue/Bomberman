#include "../Projet/config.h"
#include <ncurses.h>

void print_grid() {
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

    GridData *g = malloc(sizeof(GridData));
    if (g == NULL) {
        perror("Memory allocation error for 'g'");
        exit(EXIT_FAILURE);
    }
    g->largeur = 40;
    g->hauteur = 20;
    g->cases = calloc((g->largeur) * (g->hauteur), sizeof(int));
    if (g->cases == NULL) {
        perror("Memory allocation error for 'g->cases'");
        exit(EXIT_FAILURE);
    }
    setup_wall(g);

    player *p = malloc(4 * sizeof(player *));
    if (p == NULL) {
        perror("Memory allocation error for 'p'");
        exit(EXIT_FAILURE);
    }
    init_player(p, g);

    line *l = malloc(sizeof(line));
    if (l == NULL) {
        perror("Memory allocation error for 'l'");
        exit(EXIT_FAILURE);
    }
    l->cursor = 0;

    while (true){
        ACTION a = control(l);
        p[0].action = a;
        // envoi de l'action
        // envoi du tchat
        if(perform_action_all(p, g)) break;
        // reception du tchat
        // reception de la grille
        refresh_game(l, g);
        usleep(30 * 1000);
    }

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */
}

int main(){
    print_grid();
    return 0;
}