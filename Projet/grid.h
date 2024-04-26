 #ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <string.h>

#include "config.h"
#include "func.h"


typedef struct board {
  char *grid;
  int w; // width    ----
  int h; // height   |
} board;


line *init_grid(GridData gridData, int id, int game_mode);
int print_grid(GridData gridData, line *l);
ACTION control(line *l);
board *grid_to_board(GridData gridData);
int clear_grid();
void eraser(line *l);
bool check_end(GridData gridData);
#endif