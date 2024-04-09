#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <net/if.h>
#include <pthread.h>

#define TCP_PORT 8080
#define MULTICAST_PORT 9000
#define SIZE_MSG 1024

typedef enum ACTION { NONE, UP, DOWN, LEFT, RIGHT, BOMB, QUIT } ACTION;
enum colors { GREEN, YELLOW, MAGENTA, CYAN };

#endif