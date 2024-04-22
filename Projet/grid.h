#ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <string.h>
#include "config.h"

void refresh_game(line* l, GridData* g);
void print_grid(GridData *g, player *p, line *l);

#endif 