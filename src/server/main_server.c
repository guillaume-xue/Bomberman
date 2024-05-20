#include "server.h"

// int main() {
//   int server_socket;
//   struct sockaddr_in6 server_addr, client_addr;
//   socklen_t addr_len = sizeof(client_addr);

//   server_socket = socket(AF_INET6, SOCK_STREAM, 0);
//   if (server_socket == -1) {
//     perror("La création de la socket a échoué");
//     exit(EXIT_FAILURE);
//   }

//   memset(&server_addr, 0, sizeof(server_addr));
//   server_addr.sin6_family = AF_INET6;
//   server_addr.sin6_port = htons(TCP_PORT);
//   inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

//   if (bind(server_socket, (struct sockaddr *)&server_addr,
//            sizeof(server_addr)) < 0) {
//     perror("La liaison a échoué");
//     exit(EXIT_FAILURE);
//   }

//   if (listen(server_socket, 10) < 0) {
//     perror("Écoute des connexions a échoué");
//     exit(EXIT_FAILURE);
//   }

//   init_mutex();

//   printf("Serveur démarré, en attente de connexions...\n\n");

//   while (1) {
//     int client_socket_tcp =
//             accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

//     if (client_socket_tcp >= 0) {
//       printf("Ohhh, un client a rejoint le serveur\n");

//       int *x = malloc(sizeof(int));
//       if (!x) {
//         perror("L'allocation de la mémoire a échoué");
//         free(x);
//         exit(EXIT_FAILURE);
//       }

//       *x = client_socket_tcp;
//       pthread_t thread_client;
//       pthread_create(&thread_client, NULL, handle_client, x);

//       //pthread_join(thread_client, NULL);
//     }
//   }

//   close(server_socket);

//   return 0;
// }

int main() {
    int server_socket;
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1; 
    int timeout = 15000; // 15 secondes

    // Initialisation
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("La création de la socket a échoué");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(TCP_PORT);
    inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

    init_mutex();

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("La liaison a échoué");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("Écoute des connexions a échoué");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Serveur démarré, en attente de connexions...\n\n");

    fds[0].fd = server_socket;
    fds[0].events = POLLIN;

    while (1) {
        int poll_count = poll(fds, nfds, timeout);

        if (poll_count == -1) {
            perror("poll a échoué");
            exit(EXIT_FAILURE);
        } else if (poll_count == 0) {
            // Timeout reached
            printf("Timeout atteint, aucun événement reçu. Suppression des clients inactifs...\n");
            for (int i = 1; i < nfds; ++i) {
                if (fds[i].fd != -1) {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
            }
            nfds = 1;
            continue; // Revenir au début de la boucle
        }

        for (int i = 0; i < nfds; ++i) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_socket) {
                    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
                    if (client_socket == -1) {
                        perror("accept a échoué");
                    } else {
                        printf("Ohhh, un client a rejoint le serveur\n");

                        for (int j = 1; j < MAX_CLIENTS; ++j) {
                            if (fds[j].fd == -1) {
                                fds[j].fd = client_socket;
                                fds[j].events = POLLIN;
                                if (j >= nfds) {
                                    nfds = j + 1;
                                }
                                break;
                            }
                        }
                    }
                } else {
                    int client_socket = fds[i].fd;
                    handle_client_poll(client_socket);

                    fds[i].fd = -1;

                    if (i == nfds - 1) {
                        while (nfds > 1 && fds[nfds - 1].fd == -1) {
                            --nfds;
                        }
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}