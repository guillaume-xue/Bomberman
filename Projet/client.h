#ifndef CLIENT_H
#define CLIENT_H

#include "grid.h"
#include "server.h" 

typedef struct {
    int CODEREQ; 
    short ID;         
    int EQ;    
    int NUM;
    int ACTION;
} GameMessage; 


void send_game_request(int client_socket, int CODEREQ) ;

void receive_gmsg(int client_socket);

int join_server();

#endif