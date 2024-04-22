#ifndef GRID_HANDLER_H
#define GRID_HANDLER_H

#include <ncurses.h>
#include <signal.h>
#include "config.h"


void setup_grid(GridData *g, int hauteur, int largeur, player ** p);
void setup_wall(GridData *g);
void init_player(player ** global_players, GridData * g);
void explode_bomb(player * global_players, GridData * g);
bool perform_action_all(player ** p, GridData * g);
bool is_wall(GridData *g, int x, int y);
bool is_movable(GridData *g, int x, int y);
bool is_bomb(GridData *g, int x, int y);
bool is_wall_breakable(GridData *g, int x, int y);
void set_grid(GridData *g, int x, int y, int v);
int get_grid(GridData *g, int x, int y);
void clear_grid(GridData *g, int x, int y);
void start_game(int hauteur, int largeur);

#endif