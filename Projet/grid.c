#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "grid.h"

#define TEXT_SIZE 255

int id_p;

typedef struct board {
  char *grid;
  int w; // width    ----
  int h; // height   |
} board;

void setup_board(board *board) {
  int lines;
  int columns;
  getmaxyx(stdscr, lines, columns);
  board->h = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
  board->w = columns - 2;   // 2 columns reserved for border
  board->grid = calloc((board->w) * (board->h), sizeof(char));
}

void free_board(board *board) { free(board->grid); }

ContenuCase get_grid(board *b, int x, int y) { return b->grid[y * b->w + x]; }

void set_grid(board *b, int x, int y, ContenuCase v) {
  b->grid[y * b->w + x] = v;
}

void refresh_game(board *b, line *l) {
  int x, y;
  for (y = 0; y < b->h; y++) {
    for (x = 0; x < b->w; x++) {
      char c = get_grid(b, x, y);
      switch (c) {
      case '1':
        if (id_p == 1) {
          attron(COLOR_PAIR(1)); // Activer la couleur
          attron(A_BOLD);        // Activer le gras
          mvaddch(y + 1, x + 1, '1');
          attroff(A_BOLD);        // Désactiver le gras
          attroff(COLOR_PAIR(1)); // Désactiver la couleur
        } else {
          mvaddch(y + 1, x + 1, c); // Afficher le caractère normalement
        }
        break;

      case '2':
        if (id_p == 2) {
          attron(COLOR_PAIR(2));
          attron(A_BOLD);
          mvaddch(y + 1, x + 1, '2');
          attroff(A_BOLD);
          attroff(COLOR_PAIR(2));
        } else {
          mvaddch(y + 1, x + 1, c);
        }
        break;

      case '3':
        if (id_p == 3) {
          attron(COLOR_PAIR(3));
          attron(A_BOLD);
          mvaddch(y + 1, x + 1, '3');
          attroff(A_BOLD);
          attroff(COLOR_PAIR(3));
        } else {
          mvaddch(y + 1, x + 1, c);
        }
        break;

      case '4':
        if (id_p == 4) {
          attron(COLOR_PAIR(4));
          attron(A_BOLD);
          mvaddch(y + 1, x + 1, '4');
          attroff(A_BOLD);
          attroff(COLOR_PAIR(4));
        } else {
          mvaddch(y + 1, x + 1, c);
        }
        break;

      default:
        mvaddch(y + 1, x + 1,
                c); // Afficher le caractère normalement pour le reste
        break;
      }
    }
  }
  for (x = 0; x < b->w + 2; x++) {
    mvaddch(0, x, '-');
    mvaddch(b->h + 1, x, '-');
  }
  for (y = 0; y < b->h + 2; y++) {
    mvaddch(y, 0, '|');
    mvaddch(y, b->w + 1, '|');
  }
  // Update chat text
  attron(COLOR_PAIR(1)); // Enable custom color 1
  attron(A_BOLD);        // Enable bold
  for (x = 0; x < b->w + 2; x++) {
    if (x >= TEXT_SIZE || x >= l->cursor)
      mvaddch(b->h + 2, x, ' ');
    else
      mvaddch(b->h + 2, x, l->data[x]);
  }
  attroff(A_BOLD);        // Disable bold
  attroff(COLOR_PAIR(1)); // Disable custom color 1
  refresh();              // Apply the changes to the terminal
}

ACTION control(line *l) {
  int c;
  int prev_c = ERR;
  // We consume all similar consecutive key presses
  while ((c = getch()) !=
         ERR) { // getch returns the first key press in the queue
    if (prev_c != ERR && prev_c != c) {
      ungetch(c); // put 'c' back in the queue
      break;
    }
    prev_c = c;
  }
  ACTION a = NONE;
  switch (prev_c) {
  case ERR:
    break;
  case KEY_LEFT:
    a = LEFT;
    break;
  case KEY_RIGHT:
    a = RIGHT;
    break;
  case KEY_UP:
    a = UP;
    break;
  case KEY_DOWN:
    a = DOWN;
    break;
  case '~':
    a = QUIT;
    break;
  case KEY_BACKSPACE:
    if (l->cursor > 0)
      l->cursor--;
    break;
  default:
    if (prev_c >= ' ' && prev_c <= '~' && l->cursor < TEXT_SIZE)
      l->data[(l->cursor)++] = prev_c;
    break;
  }
  return a;
}

bool perform_action(board *b, pos *p, ACTION a) {
  int xd = 0;
  int yd = 0;
  switch (a) {
  case LEFT:
    xd = -1;
    yd = 0;
    break;
  case RIGHT:
    xd = 1;
    yd = 0;
    break;
  case UP:
    xd = 0;
    yd = -1;
    break;
  case DOWN:
    xd = 0;
    yd = 1;
    break;
  case QUIT:
    return true;
  default:
    break;
  }
  p->x += xd;
  p->y += yd;
  p->x = (p->x + b->w) % b->w;
  p->y = (p->y + b->h) % b->h;
  set_grid(b, p->x, p->y, 1);
  return false;
}

int print_grid() {
  board *b = malloc(sizeof(board));
  ;
  line *l = malloc(sizeof(line));
  l->cursor = 0;
  pos *p = malloc(sizeof(pos));
  p->x = 0;
  p->y = 0;

  // NOTE: All ncurses operations (getch, mvaddch, refresh, etc.) must be done
  // on the same thread.
  initscr();                /* Start curses mode */
  raw();                    /* Disable line buffering */
  intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
  keypad(stdscr, TRUE);     /* Required in order to get events from keyboard */
  nodelay(stdscr, TRUE);    /* Make getch non-blocking */
  noecho(); /* Don't echo() while we do getch (we will manually print characters
               when relevant) */
  curs_set(0);                             // Set the cursor to invisible
  start_color();                           // Enable colors
  init_pair(1, COLOR_YELLOW, COLOR_BLACK); // Define a new color style (text is
                                           // yellow, background is black)

  setup_board(b);
  while (true) {
    ACTION a = control(l);
    if (perform_action(b, p, a))
      break;
    refresh_game(b, l);
    // refresh_game(b,l);
    usleep(30 * 1000);
  }
  free_board(b);

  curs_set(1); // Set the cursor to visible again
  endwin();    /* End curses mode */

  free(p);
  free(l);
  free(b);

  return 0;
}

char get_grid_char(GridData gridData, int i, int j) {
  switch (gridData.cases[i][j]) {
  case CASE_VIDE:
    return ' ';
  case MUR_INDESTRUCTIBLE:
    return '#';
  case MUR_DESTRUCTIBLE:
    return 'X';
  case BOMBE:
    return 'B';
  case EXPLOSION:
    return 'E';
  case J0:
    return '1';
  case J1:
    return '2';
  case J2:
    return '3';
  case J3:
    return '4';
  default:
    return '?';
  }
}

board *grid_to_board(GridData gridData) {
  board *b = malloc(sizeof(board));
  check_malloc(b);

  b->w = gridData.width - 2 - 1;
  b->h = gridData.height - 2;
  b->grid = calloc((b->w) * (b->h), sizeof(char));
  check_malloc(b->grid);

  for (int i = 0; i < b->w; i++) {
    for (int j = 0; j < b->h; j++) {
      set_grid(b, i, j, get_grid_char(gridData, i, j));
    }
  }

  return b;
}

void init_grid(GridData gridData, int id) {

  id_p = id;

  puts("Init grid");
  board *b = grid_to_board(gridData);
  puts("Init grid");
  line *l = malloc(sizeof(line));
  l->cursor = 0;

  // NOTE: All ncurses operations (getch, mvaddch, refresh, etc.) must be done
  // on the same thread.
  initscr();                /* Start curses mode */
  raw();                    /* Disable line buffering */
  intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
  keypad(stdscr, TRUE);     /* Required in order to get events from keyboard */
  nodelay(stdscr, TRUE);    /* Make getch non-blocking */
  noecho(); /* Don't echo() while we do getch (we will manually print characters
               when relevant) */
  curs_set(0);                              // Set the cursor to invisible
  start_color();                            // Enable colors
  init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Define a new color style (text is
                                           // green, background is black)
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Define a new color style (text is
                                            // yellow, background is black)
  init_pair(3, COLOR_MAGENTA, COLOR_BLACK); // Define a new color style (text is
                                            // magenta, background is black)
  init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Define a new color style (text is
                                            // cyan, background is black)

  refresh_game(b, l);
}