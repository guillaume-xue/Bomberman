#include "client.h"

int main() {
  connexion_to_tcp_server();

  choose_game_mode();

  suscribe_multicast();

  im_ready(); // dernière étape avant de commencer la partie

  launch_game();

  // Gestion fin de partie

  return 0;
}