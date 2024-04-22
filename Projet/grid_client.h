#ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <string.h>
#include "config.h"

void refresh_game(line* l, GridData* g);
void print_grid(GridData grid, player *players, line *line);
ACTION control(line* l);

#endif 