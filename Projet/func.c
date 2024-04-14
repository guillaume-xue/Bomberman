#include "func.h"

char *id_to_color(int id) {
  switch (id - 1) {
  case 0:
    return "\33[0;32m"; // green
  case 1:
    return "\33[0;33m"; // yellow
  case 2:
    return "\33[0;35m"; // magenta
  case 3:
    return "\33[0;36m"; // cyan
  default:
    return "\33[0m"; // white
  }
}

void clear_term() {
  printf("\033[90mLe terminal sera netttoyé dans 3 secondes\033[0m\n");
  for (int i = 0; i < 3; i++) {
    printf("\033[90m%d\033[0m \n", 3 - i);
    sleep(1);
  }
  system("clear");
}