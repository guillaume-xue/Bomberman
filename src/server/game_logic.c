#include "game_logic.h"

typedef struct bomb_arg{
    int index_partie;
    int id_player;
} bomb_arg;

int get_grid(int index_partie, int x, int y) {
  return parties[index_partie].grid.cases[x][y];
}

void set_grid(int index_partie, int x, int y, ContenuCase c) {
  parties[index_partie].grid.cases[x][y] = c;
}

void clear_grid(int index_partie, int x, int y) {
  set_grid(index_partie, x, y, CASE_VIDE);
}

bool is_bomb(int index_partie, int x, int y) {
  return get_grid(index_partie, x, y) == BOMBE;
}

// Retourne vrai si la case est un mur cassable
bool is_wall_breakable(int index_partie, int x, int y){
  return get_grid(index_partie, x, y) == MUR_DESTRUCTIBLE;
}

// Retourne vrai si la case est un mur
bool is_wall(int index_partie, int x, int y) {
  return get_grid(index_partie, x, y) == MUR_DESTRUCTIBLE || get_grid(index_partie, x, y) == MUR_INDESTRUCTIBLE;
}

bool is_vide(int index_partie, int x, int y) {
  return get_grid(index_partie, x, y) == CASE_VIDE;
}

bool is_player(int index_partie, int x, int y) {
  for (int i = 5; i <= 8; ++i) {
    if (get_grid(index_partie, x, y) == i) return true;
  }
  return false;
}

bool is_movable(int index_partie, int x, int y) {
  return x >= 0 && x <= FIELD_WIDTH - 1 &&
         y >= 0 && y <= FIELD_HEIGHT - 1 &&
         !is_wall(index_partie, x, y) && !is_bomb(index_partie, x, y) &&
         is_player(index_partie, x, y) == false;
}

bool is_exploding(int index_partie, int x, int y) {
  return get_grid(index_partie, x, y) == EXPLOSION;
}

// Place les murs sur la grille
void setup_wall(int index_partie) {
  // On met des murs incassables sur les cases impaires
  for (int i = 1; i < parties[index_partie].grid.width; i+=2) {
    for (int j = 1; j < parties[index_partie].grid.height; j+=2) {
      set_grid(index_partie, i, j, MUR_INDESTRUCTIBLE);
    }
  }
  // On met des murs cassables aléatoirement
  int i = 0;
  while (i < NB_WALLS) {
    int x = rand() % parties[index_partie].grid.width;
    int y = rand() % parties[index_partie].grid.height;
    if (get_grid(index_partie, x, y) == CASE_VIDE){
      set_grid(index_partie, x, y, MUR_DESTRUCTIBLE);
      i++;
    }
  }
}

// Fait exploser la bombe
void explode_bombe(int index_partie, int id_player){
  // On met les explosions
  for (int i = parties[index_partie].players[id_player].b.x - 1; i <= parties[index_partie].players[id_player].b.x + 1; i++) {
    for (int j = parties[index_partie].players[id_player].b.y - 1; j <= parties[index_partie].players[id_player].b.y + 1; j++) {
      // On casse les murs cassables
      if (i >= 0 && i < parties[index_partie].grid.width &&
          j >= 0 && j < parties[index_partie].grid.height &&
          (is_wall_breakable(index_partie, i, j) || is_vide(index_partie, i, j))){
        set_grid(index_partie, i, j, EXPLOSION);
      }
      // On tue les joueurs
      if(is_player(index_partie, i, j)){
        int id = get_grid(index_partie, i, j) - 5;
        parties[index_partie].players[id].dead = true;
        set_grid(index_partie, i, j, EXPLOSION);
      }
    }
  }
  sleep(1); // Temps d'affichage de l'explosion
  // On efface la bombe et les explosions
  for (int i = parties[index_partie].players[id_player].b.x - 1; i <= parties[index_partie].players[id_player].b.x + 1; i++) {
    for (int j = parties[index_partie].players[id_player].b.y - 1; j <= parties[index_partie].players[id_player].b.y + 1; j++) {
      if (is_exploding(index_partie, i, j) || is_bomb(index_partie, i, j)){
        clear_grid(index_partie, i, j);
      }
    }
  }
  parties[index_partie].players[id_player].b.set = false;
}

// Fonction appelée par le thread pour faire exploser la bombe
void *explose_handler(void * arg) {
  sleep(3);
  int index_partie = ((bomb_arg *)arg)->index_partie;
  int id_player = ((bomb_arg *)arg)->id_player;
  explode_bombe(index_partie, id_player);
  free(arg);
  return NULL;
}

int place_bomb(int index_partie, int id_player, pos p) {
  if (parties[index_partie].players[id_player].b.set == true) return 1; // Si le joueur a déjà une bombe

  parties[index_partie].players[id_player].b.x = p.x;
  parties[index_partie].players[id_player].b.y = p.y;

  pthread_t thread;
  bomb_arg *arg = malloc(sizeof(bomb_arg));
  arg->index_partie = index_partie;
  arg->id_player = id_player;
  parties[index_partie].players[id_player].b.set = true;
  if (pthread_create(&thread, NULL, explose_handler, arg) != 0){
    fprintf(stderr, "Erreur lors de la création du thread.\n");
    return 1;
  }

  return 0;
}

// On vérifie si l'action du joueur est legit
int check_maj(GameMessage *game_message, Partie *partie) {
  int id = game_message->ID - 1;
  // Si le joueur est mort, on ne fait rien
  if (partie->players[id].dead == true) {
    return -1;
  }
  ACTION action = game_message->ACTION;

  pos p = partie->players[id].p;
  switch (action) {
    case UP:
      if (is_movable(partie->partie_id, p.x, p.y - 1) == false)
        return -1;
      else if (partie->grid.cases[p.x][p.y - 1] == MUR_DESTRUCTIBLE) {
        return -1;
      } else {
        partie->players[id].p.y = p.y - 1;
        partie->grid.cases[p.x][p.y] = CASE_VIDE;
        partie->grid.cases[p.x][p.y - 1] = id + 5;
      }
      break;
    case DOWN:
      if (is_movable(partie->partie_id, p.x, p.y + 1) == false) {
        return -1;
      }else if (partie->grid.cases[p.x][p.y + 1] == MUR_DESTRUCTIBLE) {
        return -1;
      } else {
        partie->players[id].p.y = p.y + 1;
        partie->grid.cases[p.x][p.y] = CASE_VIDE;
        partie->grid.cases[p.x][p.y + 1] = id + 5;
      }
      break;
    case LEFT:
      if (is_movable(partie->partie_id, p.x - 1, p.y) == false)
        return -1;

      else if (partie->grid.cases[p.x - 1][p.y] == MUR_DESTRUCTIBLE) {
        return -1;
      } else {
        partie->players[id].p.x = p.x - 1;
        partie->grid.cases[p.x][p.y] = CASE_VIDE;
        partie->grid.cases[p.x - 1][p.y] = id + 5;
      }
      break;// détecter si un joueur est touché par l'explosion
    case RIGHT:
      if (is_movable(partie->partie_id, p.x + 1, p.y) == false)
        return -1;
      else if (partie->grid.cases[p.x + 1][p.y] == MUR_DESTRUCTIBLE) {
        return -1;
      } else {
        partie->players[id].p.x = p.x + 1;
        partie->grid.cases[p.x][p.y] = CASE_VIDE;
        partie->grid.cases[p.x + 1][p.y] = id + 5;
      }
      break;
    case BOMB:
      if(place_bomb(partie->partie_id, id, p) == 1) return -1;
      break;
    case QUIT:
      break;
    default:
      return -1;
  }
  if (partie->players[id].b.set == true) {
    set_grid(partie->partie_id, partie->players[id].b.x, partie->players[id].b.y, BOMBE);
  }
  return 0;
}