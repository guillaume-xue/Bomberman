#ifndef CLIENT_H
#define CLIENT_H

void send_game_request(int client_socket, int CODEREQ);
void connexion_to_tcp_server();
void choose_game_mode();
void *handle_udp(void *arg);
char *str_to_color(char *str);
void suscribe_multicast();
void receive_gmsg(int client_socket);

#endif