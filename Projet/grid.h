#ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

#define TEXT_SIZE 255
#define NB_WALLS 100

typedef struct board {
    char* grid;
    int largeur;
    int hauteur;
} board;

typedef struct line {
    char data[TEXT_SIZE];
    int cursor;
} line;

typedef struct pos {
    int x;
    int y;
} pos;

typedef struct bomb {
    int x;
    int y;
    bool set;
} bomb;

typedef struct player{
    GameMessage* msg;
    pos *position;
    bomb* bombe;
} player;

bool is_wall(board* b, int x, int y);
bool is_movable(board* b, int x, int y);
bool is_bomb(board* b, int x, int y);
bool is_wall_breakable(board* b, int x, int y);
void set_grid(board* b, int x, int y, int v);
void explode_bomb(board* b, player* p);
void alarm_handler(int signum);
int get_grid(board* b, int x, int y);
int grid_creation();

#endif 