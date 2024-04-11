#ifndef GRID_HANDLER_H
#define GRID_HANDLER_H

#include <ncurses.h>
#include <signal.h>
#include "config.h"

bool is_wall(int x, int y);
bool is_movable(int x, int y);
bool is_bomb(int x, int y);
bool is_wall_breakable(int x, int y);
void set_grid(int x, int y, int v);
int get_grid(int x, int y);
void explode_bomb();
void alarm_handler(int signum);

#endif