#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "grid.h"

int joueur_id;
int joueur_mode;
pos *p;
char notif[TEXT_SIZE];

void free_board(board *board) { free(board->grid); }

ContenuCase get_grid(board *b, int x, int y) { return b->grid[y * b->w + x]; }

void set_grid(board *b, int x, int y, ContenuCase v) {
  b->grid[y * b->w + x] = v;
}

void refresh_game(board *b, line *l) {
  int x, y;
  for (y = 0; y < FIELD_HEIGHT; y++) {
    for (x = 0; x < FIELD_WIDTH; x++) {
      char c = get_grid(b, x, y);
      switch (c) {
      case '1':
        if (joueur_id == 1) {
          attron(COLOR_PAIR(1)); // Activer la couleur
          attron(A_BOLD);        // Activer le gras
          mvaddch(y + 2, x + 1, '1');
          attroff(A_BOLD);        // Désactiver le gras
          attroff(COLOR_PAIR(1)); // Désactiver la couleur
        } else {
          mvaddch(y + 2, x + 1, c); // Afficher le caractère normalement
        }
        break;

      case '2':
        if (joueur_id == 2) {
          attron(COLOR_PAIR(2));
          attron(A_BOLD);
          mvaddch(y + 2, x + 1, '2');
          attroff(A_BOLD);
          attroff(COLOR_PAIR(2));
        } else {
          mvaddch(y + 2, x + 1, c);
        }
        break;

      case '3':
        if (joueur_id == 3) {
          attron(COLOR_PAIR(3));
          attron(A_BOLD);
          mvaddch(y + 2, x + 1, '3');
          attroff(A_BOLD);
          attroff(COLOR_PAIR(3));
        } else {
          mvaddch(y + 2, x + 1, c);
        }
        break;

      case '4':
        if (joueur_id == 4) {
          attron(COLOR_PAIR(4));
          attron(A_BOLD);
          mvaddch(y + 2, x + 1, '4');
          attroff(A_BOLD);
          attroff(COLOR_PAIR(4));
        } else {
          mvaddch(y + 2, x + 1, c);
        }
        break;
      case 'B':
          attron(COLOR_PAIR(5)); 
          attron(A_BOLD);        
          mvaddch(y + 2, x + 1, 'B');
          attroff(A_BOLD);        
          attroff(COLOR_PAIR(5)); 
          break;
      case '*':
          attron(COLOR_PAIR(5));
          attron(A_BOLD);        
          mvaddch(y + 2, x + 1, '*');
          attroff(A_BOLD);       
          attroff(COLOR_PAIR(5)); 
          break;

      default:
        mvaddch(y + 2, x + 1, c);
        break;
      }
    }
  }

  char buf_player[TEXT_SIZE];
  snprintf(buf_player, sizeof(buf_player), "Joueur %d", joueur_id);
  size_t len_player = (FIELD_WIDTH / 2) - (strlen(buf_player) / 2);
  for (int i = 1; i < len_player + 1; i++)
    mvaddch(0, i, '-');
  attron(COLOR_PAIR(joueur_id));
  attron(A_BOLD);
  mvprintw(0, len_player, buf_player);
  attroff(A_BOLD);
  attroff(COLOR_PAIR(joueur_id));
  for (int i = len_player + strlen(buf_player); i < FIELD_WIDTH + 1; i++)
    mvaddch(0, i, '-');
  mvaddch(0, b->w + 2, '|');

  for (x = 0; x < b->w + 2; x++) {
    mvaddch(1, x, '-');
    mvaddch(b->h + 2, x, '-');
  }
  for (y = 0; y < b->h + 2; y++) {
    mvaddch(y, 0, '|');
    mvaddch(y + 1, b->w + 1, '|');
  }

  // ---------------------TCHATBOX---------------------
  // ------TCHATBOX---------------
  int nh = b->h + 3; // new height
  size_t len_tb = (FIELD_WIDTH / 2) - (strlen("TCHATBOX") / 2);
  for (int i = 1; i < len_tb + 1; i++)
    mvaddch(nh, i, '-');
  mvprintw(nh, len_tb, "TCHATBOX");
  for (int i = len_tb + strlen("TCHATBOX"); i < FIELD_WIDTH + 1; i++)
    mvaddch(nh, i, '-');

  for (int i = 0; i < l->tchatbox.nb_lines; i++) {
    mvprintw(nh + 1 + i, 1, l->tchatbox.conv[i]);
  }

  int height_tb = TCHATBOX_HEIGHT + nh;

  attron(COLOR_PAIR(5)); // Enable custom color 5
  attron(A_BOLD);        // Enable bold
  for (int i = 1; i < FIELD_WIDTH + 4; i++)
    mvaddch(height_tb - 1, i, ' ');
  mvprintw(height_tb - 1, 1, notif);
  attroff(A_BOLD);        // Disable bold
  attroff(COLOR_PAIR(5)); // Disable custom color 5

  for (y = nh; y < height_tb; y++) {
    mvaddch(y, 0, '|');
    mvaddch(y, b->w + 1, '|');
  }
  mvaddch(height_tb, 0, '|');
  mvaddch(height_tb, b->w + 1, '|');
  for (x = 1; x < b->w + 1; x++)
    mvaddch(height_tb, x, '-');

  for (int i = 1; i < b->w; i++)
    mvaddch(height_tb - 2, i, '-');
  // ---------------------TCHATBOX---------------------

  // ---------------------INPUT---------------------
  for (int i = 0; i < b->w; i++) {
    attron(COLOR_PAIR(joueur_id)); // Enable custom color joueur_id
    attron(A_BOLD);                // Enable bold
    if (i >= TEXT_SIZE || i >= l->cursor)
      mvaddch(height_tb + 2, i, ' ');
    else
      mvaddch(height_tb + 2, i, l->data[i]);
    attroff(A_BOLD);                // Disable bold
    attroff(COLOR_PAIR(joueur_id)); // Disable custom color joueur_id
    if (i == l->cursor)
      mvaddch(height_tb + 2, i, '|');
  }

  mvprintw(height_tb + 4, 2, "Exemple : '7bla bla bla'");
  mvprintw(height_tb + 5, 2, "-'@': tout effacer.");
  mvprintw(height_tb + 6, 2, "-'~': quitter.");
  mvprintw(height_tb + 7, 2, "-'7': send everyone.");
  if (joueur_mode == 2)
    mvprintw(height_tb + 8, 2, "-'8': send teammate.");
  // ---------------------INPUT---------------------

  refresh(); // Apply the changes to the terminal
}

void eraser(line *l) {
  memset(l->data, 0, TEXT_SIZE);
  l->cursor = 0;
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
  case 'b':
    a = BOMB;
    break;
  case KEY_BACKSPACE:
    if (l->cursor > 0)
      l->cursor--;
    break;
  case 127: // ASCII code pour backspace, KEY_BACKSPACE marche pas sur macbook
    if (l->cursor > 0)
      l->data[(l->cursor)--] = '\0';
    break;
  case '@': // Pour tout effacer
    eraser(l);
    break;
  case '\n': // KEY_ENTER
    if (l->data[0] != '\0' && l->cursor < TEXT_SIZE &&
        (l->data[0] == '7' || (l->data[0] == '8' && joueur_mode == 2))) {
      memset(notif, 0, sizeof(notif));
      snprintf(notif, sizeof(notif), "Message envoyé !");
      a = SUBMIT;
    } else {
      eraser(l);
      memset(notif, 0, sizeof(notif));
      snprintf(notif, sizeof(notif), "Erreur : message invalide.");
    }
    break;
  default:
    if (prev_c >= ' ' && prev_c <= '~' && l->cursor < FIELD_WIDTH - 1)
      l->data[(l->cursor)++] = prev_c;
    break;
  }
  return a;
}


int print_grid(GridData gridData, line *l) {
  board *b = grid_to_board(gridData);
  refresh_game(b, l);
  return 0;
}

char get_grid_char(GridData gridData, int i, int j) {
  switch (gridData.cases[i][j]) {
  case CASE_VIDE:
    return ' ';
  case MUR_INDESTRUCTIBLE:
        return 'O';
    case MUR_DESTRUCTIBLE:
        return 'x';
  case BOMBE:
    return 'B';
  case EXPLOSION:
    return '*';
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

  b->w = FIELD_WIDTH;
  b->h = FIELD_HEIGHT;
  b->grid = calloc((b->w) * (b->h), sizeof(char));
  check_malloc(b->grid);

  for (int i = 0; i < b->w; i++) {
    for (int j = 0; j < b->h; j++) {
      set_grid(b, i, j, get_grid_char(gridData, i, j));
    }
  }

  return b;
}

pos *init_position(GridData gridData) {
  pos *p = malloc(sizeof(pos));
  check_malloc(p);

  switch (joueur_id) {
  case 1:
    p->x = 0;
    p->y = 0;
    break;
  case 2:
    p->x = gridData.width - 3;
    p->y = 0;
    break;
  case 3:
    p->x = 0;
    p->y = gridData.height - 2;
    break;
  case 4:
    p->x = gridData.width - 3;
    p->y = gridData.height - 2;
    break;
  default:
    break;
  }

  return p;
}

int clear_grid() {
  clear();
  refresh();
  endwin();
  return 0;
}

line *init_grid(GridData gridData, int id, int game_mode) {

  joueur_id = id;
  joueur_mode = game_mode;

  board *b = grid_to_board(gridData);

  line *l = malloc(sizeof(line));
  check_malloc(l);
  memset(l->data, 0, TEXT_SIZE);
  memset(l->tchatbox.conv, 0, TEXT_SIZE);
  l->tchatbox.i = 0;
  l->cursor = 0;

  p = init_position(gridData);

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
  init_pair(1, COLOR_GREEN, COLOR_BLACK);   // color player 1
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // color player 2
  init_pair(3, COLOR_MAGENTA, COLOR_BLACK); // color player 3
  init_pair(4, COLOR_CYAN, COLOR_BLACK);    // color player 4
  init_pair(5, COLOR_RED, COLOR_BLACK);     // color bomb

  refresh_game(b, l);

  free_board(b);
  free(b);

  return l;
}