#ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <signal.h>
#include "config.h"
#include "client.h"
#include "grid_handler.h"

int print_grid(player *current_player, player **players);

#endif 