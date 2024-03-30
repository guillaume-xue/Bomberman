#include "config.h"
#include "grid.h"

typedef struct {
    char CODEREQ[12]; 
    short ID;         
    char EQ;    
    char NUM[12];
    int ACTION;
} GameMessage; 

void send_game_request(int client_socket, char* CODEREQ) ;
void send_chat_message(int client_socket, char* CODEREQ, short ID, char EQ, char *message) ;
void receive_message(int client_socket, void *buffer, size_t buffer_size);

int join_server();