#ifndef GRID_H
#define GRID_H

#include "config.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEXT_SIZE 255
#define NB_WALLS 10
#define BOMB 4

enum colors { GREEN, YELLOW, MAGENTA, CYAN };

typedef struct {
  int x, y;      // Position de la bombe
  int countdown; // Compte à rebours avant explosion
} bomb;

typedef struct board {
  char *grid;
  int largeur;
  int hauteur;
  bomb *bombs;
  int nb_bombs;
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