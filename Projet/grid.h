#ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

#define TEXT_SIZE 255
#define NB_WALLS 10

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

typedef struct player {
    pos *p;
    int id;
    bomb *b;
    GameMessage *msg;
    ACTION action;
} player;

bool is_wall(int x, int y);
bool is_movable(int x, int y);
bool is_bomb(int x, int y);
bool is_wall_breakable(int x, int y);
void set_grid(int x, int y, int v);
void explode_bomb();
void alarm_handler(int signum);
int get_grid(int x, int y);
int grid_creation(int actual_player);

#endif 