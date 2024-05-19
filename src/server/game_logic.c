#include "game_logic.h"

typedef struct bomb_arg{
    Partie *partie;
    int id_player;
    FreqMessage *freqMessage;
} bomb_arg;

int get_grid(Partie *partie, int x, int y) {
  return partie->grid.cases[x][y];
}

void set_grid(Partie *partie, int x, int y, ContenuCase c) {
  partie->grid.cases[x][y] = c;
}

void clear_grid(Partie *partie, int x, int y) {
  set_grid(partie, x, y, CASE_VIDE);
}

bool is_bomb(Partie *partie, int x, int y) {
  return get_grid(partie, x, y) == BOMBE;
}

// Retourne vrai si la case est un mur cassable
bool is_wall_breakable(Partie *partie, int x, int y){
  return get_grid(partie, x, y) == MUR_DESTRUCTIBLE;
}

// Retourne vrai si la case est un mur
bool is_wall(Partie *partie, int x, int y) {
  return get_grid(partie, x, y) == MUR_DESTRUCTIBLE || get_grid(partie, x, y) == MUR_INDESTRUCTIBLE;
}

bool is_vide(Partie *partie, int x, int y) {
  return get_grid(partie, x, y) == CASE_VIDE;
}

bool is_player(Partie *partie, int x, int y) {
  for (int i = 5; i <= 8; ++i) {
    if (get_grid(partie, x, y) == i) return true;
  }
  return false;
}

bool is_movable(Partie *partie, int x, int y) {
  return x >= 0 && x <= FIELD_WIDTH - 1 &&
         y >= 0 && y <= FIELD_HEIGHT - 1 &&
         !is_wall(partie, x, y) && !is_bomb(partie, x, y) &&
         is_player(partie, x, y) == false;
}

bool is_exploding(Partie *partie, int x, int y) {
  return get_grid(partie, x, y) == EXPLOSION;
}

// Place les murs sur la grille
void setup_wall(Partie *partie) {
  // On met des murs incassables sur les cases impaires
  for (int i = 1; i < partie->grid.width; i+=2) {
    for (int j = 1; j < partie->grid.height; j+=2) {
      set_grid(partie, i, j, MUR_INDESTRUCTIBLE);
    }
  }
  // On met des murs cassables aléatoirement
  int i = 0;
  while (i < NB_WALLS) {
    int x = rand() % partie->grid.width;
    int y = rand() % partie->grid.height;
    if (get_grid(partie, x, y) == CASE_VIDE){
      set_grid(partie, x, y, MUR_DESTRUCTIBLE);
      i++;
    }
  }
}

// Fait exploser la bombe
void explode_bombe(Partie *partie, int id_player, FreqMessage *freqMessage){
  // On met les explosions
  for (int i = partie->players[id_player].b.x - 1; i <= partie->players[id_player].b.x + 1; i++) {
    for (int j = partie->players[id_player].b.y - 1; j <= partie->players[id_player].b.y + 1; j++) {
      // On casse les murs cassables
      if (i >= 0 && i < partie->grid.width &&
          j >= 0 && j < partie->grid.height &&
          (is_wall_breakable(partie, i, j) || is_vide(partie, i, j))){
        set_grid(partie, i, j, EXPLOSION);
        freqMessage->DATA[freqMessage->NB] = j;
        freqMessage->DATA[freqMessage->NB + 1] = i;
        freqMessage->DATA[freqMessage->NB + 2] = EXPLOSION;
        freqMessage->NB++;
      }
      // On tue les joueurs
      if(is_player(partie, i, j)){
        int id = get_grid(partie, i, j) - 5;
        partie->players[id].dead = true;
        set_grid(partie, i, j, EXPLOSION);
        freqMessage->DATA[freqMessage->NB] = j;
        freqMessage->DATA[freqMessage->NB + 1] = i;
        freqMessage->DATA[freqMessage->NB + 2] = EXPLOSION;
        freqMessage->NB++;
      }
    }
  }
  sleep(1); // Temps d'affichage de l'explosion
  // On efface la bombe et les explosions
  for (int i = partie->players[id_player].b.x - 1; i <= partie->players[id_player].b.x + 1; i++) {
    for (int j = partie->players[id_player].b.y - 1; j <= partie->players[id_player].b.y + 1; j++) {
      if (is_exploding(partie, i, j) || is_bomb(partie, i, j)){
        clear_grid(partie, i, j);
        freqMessage->DATA[freqMessage->NB] = j;
        freqMessage->DATA[freqMessage->NB + 1] = i;
        freqMessage->DATA[freqMessage->NB + 2] = CASE_VIDE;
        freqMessage->NB++;
      }
    }
  }
  partie->players[id_player].b.set = false;
}

// Fonction appelée par le thread pour faire exploser la bombe
void *explose_handler(void * arg) {
  sleep(3);
  Partie *partie = ((bomb_arg *)arg)->partie;
  int id_player = ((bomb_arg *)arg)->id_player;
  FreqMessage *freqMessage = ((bomb_arg *)arg)->freqMessage;
  explode_bombe(partie, id_player,freqMessage);
  free(arg);
  return NULL;
}

int place_bomb(Partie *partie, int id_player, pos p, FreqMessage *freqMessage) {
  if (partie->players[id_player].b.set == true) return 1; // Si le joueur a déjà une bombe

  partie->players[id_player].b.x = p.x;
  partie->players[id_player].b.y = p.y;

  pthread_t thread;
  bomb_arg *arg = malloc(sizeof(bomb_arg));
  arg->partie = partie;
  arg->id_player = id_player;
  arg->freqMessage = freqMessage;
  partie->players[id_player].b.set = true;
  if (pthread_create(&thread, NULL, explose_handler, arg) != 0){
    fprintf(stderr, "Erreur lors de la création du thread.\n");
    return 1;
  }

  return 0;
}

// On vérifie si l'action du joueur est legit
int check_maj(GameMessage *game_message, Partie *partie, FreqMessage *freq_message) {
  int id = game_message->ID - 1;
  // Si le joueur est mort, on ne fait rien
  if (partie->players[id].dead == true) {
    return -1;
  }
  ACTION action = game_message->ACTION;
  int x = 0;
  int y = 0;

  pos p = partie->players[id].p;
  switch (action) {
    case UP:
      y = -1;
      break;
    case DOWN:
      y = 1;
      break;
    case LEFT:
      x = -1;
      break;// détecter si un joueur est touché par l'explosion
    case RIGHT:
      x = 1;
      break;
    case BOMB:
      if(place_bomb(partie, id, p, freq_message) == 1) return -1;
      break;
    case QUIT:
      break;
    default:
      return -1;
  }

  if (is_movable(partie, p.x + x, p.y - y) == false)
    return -1;
  else if (partie->grid.cases[p.x + y][p.y + y] == MUR_DESTRUCTIBLE) {
    return -1;
  } else {
    partie->players[id].p.y = p.y + y;
    partie->players[id].p.x = p.x + x;
    partie->grid.cases[p.x][p.y] = CASE_VIDE;
    freq_message->DATA[freq_message->NB] = p.y;
    freq_message->DATA[freq_message->NB + 1] = p.x;
    freq_message->DATA[freq_message->NB + 2] = CASE_VIDE;
    freq_message->NB++;
    partie->grid.cases[p.x + x][p.y + y] = id + 5;
    freq_message->DATA[freq_message->NB] = p.y + y;
    freq_message->DATA[freq_message->NB + 1] = p.x + x;
    freq_message->DATA[freq_message->NB + 2] = id + 5;
    freq_message->NB++;
  }

  if (partie->players[id].b.set == true) {
    set_grid(partie, partie->players[id].b.x, partie->players[id].b.y, BOMBE);
  }
  return 0;
}