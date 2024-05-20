#ifndef BOMBERMAN_GAME_LOGIC_H
#define BOMBERMAN_GAME_LOGIC_H

#include "server.h"

void setup_wall(Partie *partie);
int check_maj(GameMessage *game_message, Partie *partie, FreqGrid *freq_grid);

#endif //BOMBERMAN_GAME_LOGIC_H
