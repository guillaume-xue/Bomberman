#ifndef GRID_H
#define GRID_H

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define TEXT_SIZE 255
#define NB_WALLS 10

enum colors { GREEN, YELLOW, MAGENTA, CYAN };


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

int grid_creation();

#endif 