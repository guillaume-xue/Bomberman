 #ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <string.h>

#include "config.h"
#include "func.h"

void init_grid(GridData gridData, int id);
int print_grid();

#endif 