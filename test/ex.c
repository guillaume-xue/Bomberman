#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define SERVER_PORT 12121
#define MULTICAST_PORT 10201
#define MULTICAST_GROUP "ff02::1:2:3"
#define SERVER_IP "::1"

void print_udp_subscription(int sockfd) {
    struct ipv6_mreq mreq;
    socklen_t len = sizeof(mreq);
  
    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &mreq, &len) == 0) {
        char multicast_group[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(mreq.ipv6mr_multiaddr), multicast_group, INET6_ADDRSTRLEN);
        printf("Le socket est abonné au groupe multicast : %s\n", multicast_group);
    } else {
        perror("getsockopt(IPV6_JOIN_GROUP) failed");
    }
}

// SERVER
int ex1() {
    int listen_sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (listen_sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(SERVER_PORT);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Initialisation de socket UDP pour envoyer des messages multicast
    int send_sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (send_sock < 0) {
        perror("socket for sending failed");
        exit(EXIT_FAILURE);
    }

    // Initialisation de l'adresse de multicast
    struct sockaddr_in6 multi_addr;
    memset(&multi_addr, 0, sizeof(multi_addr));
    multi_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, MULTICAST_GROUP, &multi_addr.sin6_addr);
    multi_addr.sin6_port = htons(MULTICAST_PORT);

    int ifindex = if_nametoindex("wlp0s20f3");
    if (ifindex == 0) {
        perror("if_nametoindex");
        close(send_sock);
        return 1;
    }

    // Initialisation de l'interface locale autorisant le multicast IPv6
    if (setsockopt(send_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
        perror("setsockopt");
        close(send_sock);
        return 1;
    }

    printf("Server is listening on port %d\n", SERVER_PORT);
    printf(" Server group %s \n" , MULTICAST_GROUP);
    print_udp_subscription(listen_sock);
    char buffer[MAX_SIZE];
    char msg_to_send[MAX_SIZE];
    time_t now;
    struct tm *now_tm;
    char time_str[9];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        struct sockaddr_in6 client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int n = recvfrom(listen_sock, buffer, MAX_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (n < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        buffer[n] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            break;
        } else {
            printf("Message received: %s\n", buffer);
        }

        time(&now);
        now_tm = localtime(&now);
        strftime(time_str, sizeof(time_str), "%H:%M:%S", now_tm);

        char addr_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &client_addr.sin6_addr, addr_str, INET6_ADDRSTRLEN);

        snprintf(msg_to_send, sizeof(msg_to_send), "%s\t%s\t%s", addr_str, time_str, buffer);

        if (sendto(send_sock, msg_to_send, strlen(msg_to_send), 0, (struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
    }

    close(listen_sock);
    close(send_sock);

    return 0;
}

void *listen_msg(void *arg) {
    int sock = *(int *)arg;
    char buf[MAX_SIZE];
    while (1) {
        memset(buf, 0, sizeof(buf));
        if (recv(sock, buf, sizeof(buf), 0) < 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }

        printf("Message received from server: %s\n", buf);
    }

    return NULL;
}

// CLIENT
int ex2() {
    // ------------------------------
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Plusieurs instances locales de cette application écoutant sur le port multicast et recevant chacune les différents paquets
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    /* Initialisation de l'adresse de reception */
    struct sockaddr_in6 receive_addr;
    memset(&receive_addr, 0, sizeof(receive_addr));
    receive_addr.sin6_family = AF_INET6;
    receive_addr.sin6_addr = in6addr_any;
    receive_addr.sin6_port = htons(MULTICAST_PORT);

    if (bind(sock, (struct sockaddr *)&receive_addr, sizeof(receive_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // ------------------------------

    // ------------------------------
    // S'abonner au groupe multicast
    struct ipv6_mreq group;
    group.ipv6mr_interface = if_nametoindex("wlp0s20f3");
    inet_pton(AF_INET6, MULTICAST_GROUP, &group.ipv6mr_multiaddr.s6_addr);

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    // ------------------------------

    // ------------------------------
    // adresse de destination
    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, SERVER_IP, &server_addr.sin6_addr);
    server_addr.sin6_port = htons(SERVER_PORT);
    // ------------------------------

    pthread_t thread_client;
    int *x = malloc(sizeof(int));
    *x = sock;
    pthread_create(&thread_client, NULL, listen_msg, (void *)x);

    char buf[MAX_SIZE];
    printf("Client is listening on port %d\n", SERVER_PORT);
    printf(" Client group %s \n" , MULTICAST_GROUP);
    print_udp_subscription(sock);
    
    while (1) {
        memset(buf, 0, sizeof(buf));
        puts("Enter a message to send:");
        scanf("%s", buf);

        if (sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }

        if (strcmp(buf, "exit") == 0) {
            break;
        }
    }

    close(sock);
    return 0;
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: ./dif6 <message>\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "server") == 0) {
        ex1();
    } else if (strcmp(argv[1], "client") == 0) {
        ex2();
    } else {
        fprintf(stderr, "usage: ./dif6 <server|client>\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}