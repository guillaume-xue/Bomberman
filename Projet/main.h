#ifndef BOMBERMAN_MAIN_H
#define BOMBERMAN_MAIN_H

#define TEXT_SIZE 255

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

bool is_wall(int x, int y);
bool is_movable(int x, int y);
bool is_bomb(int x, int y);
bool is_wall_breakable(int x, int y);
void set_grid(int x, int y, int v);
void explode_bomb();
void alarm_handler(int signum);
int get_grid(int x, int y);


#endif //BOMBERMAN_MAIN_H
