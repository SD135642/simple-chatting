#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "util.h"

// Max number of concurrent clients.
#define MAX_CONNECTIONS 3

int server_socket = -1, num_connections = 0;
int client_sockets[MAX_CONNECTIONS];

char inbuf[MAX_MSG_LEN + 1];
char outbuf[BUFLEN + 1];
char *usernames[MAX_CONNECTIONS];

struct sockaddr_in server_addr;
socklen_t addrlen= sizeof(struct sockaddr_in);

volatile sig_atomic_t running = true;

//Signal handler.

void catch_signal(int sig) {
    running = false;
}

//Prints a header with date/time information.
void print_date_time_header(FILE *output) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    strftime(s, sizeof(s), "%c", tm);
    fprintf(output, "%s: ", s);
}

void broadcast_buffer(int skip_index, char *buf) {
    size_t bytes_to_send = strlen(buf) + 1;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (i != skip_index && client_sockets[i] != -1) {
            if (send(client_sockets[i], buf, bytes_to_send, 0) == -1) {
                printf("%d\n", client_sockets[i]);
                print_date_time_header(stderr);
                fprintf(stderr,
                    "Warning: Failed to broadcast message. %s.\n",
                    strerror(errno));
            }
        }
    }
}

//string comparator for qsort
int str_cmp(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

void create_welcome_msg(void) {
    outbuf[0] = '\0';
    strcat(outbuf, "*** Welcome to AP Chat Server v1.0 ***");
    if (num_connections == 0) {
        strcat(outbuf, "\n\nNo other users are in the chat room.");
        return;
    }
    char *names[MAX_CONNECTIONS];
    int j = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (usernames[i]) {
            names[j++] = usernames[i];
        }
    }
    qsort(names, j, sizeof(char *), str_cmp);
    strcat(outbuf, "\n\nConnected users: [");
    strcat(outbuf, names[0]);

    for (int i = 1; i < j; i++) {
        strcat(outbuf, ", ");
        strcat(outbuf, names[i]);
    }
    strcat(outbuf, "]");
}

void cleanup(void) {
    sprintf(outbuf, "bye");
    broadcast_buffer(-1, outbuf);
    //gives some time to allow clients to close first
    usleep(100000);

    //F_GETFD - returns the file descriptor flag
    if (fcntl(server_socket, F_GETFD) != -1) {
        close(server_socket);
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (fcntl(client_sockets[i], F_GETFD) != -1) {
            close(client_sockets[i]);
        }
        if (usernames[i]) {
            free(usernames[i]);
        }
    }
}

void disconnect_client(int index, char *ip, int port) {
    print_date_time_header(stdout);
    printf("Host [%s:%d] disconnected.\n", ip, port);

    sprintf(outbuf, "User [%s] left the chat room.", usernames[index]);
    broadcast_buffer(index, outbuf);

    close(client_sockets[index]);
    client_sockets[index] = -1;

    free(usernames[index]);
    usernames[index] = NULL;

    num_connections--;
}

int handle_server_socket(void) {
    int new_socket = accept(server_socket, (struct sockaddr *)&server_addr, &addrlen);
    if (new_socket < 0 && errno != EINTR) {
        fprintf(stderr, "Error: Failed to accept incoming connection. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (num_connections >= MAX_CONNECTIONS) {
        print_date_time_header(stdout);
        printf("Connection from [%s:%d] refused.\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
        close(new_socket);
        return EXIT_SUCCESS;
    }
    char connection_str[24];
    sprintf(connection_str, "[%s:%d] refused.\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    print_date_time_header(stdout);
    printf("New connection from %s.\n", connection_str);

    create_welcome_msg();
    if (send(new_socket, outbuf, strlen(outbuf) + 1, 0) == -1 &&
        errno != EINTR) {
        print_date_time_header(stderr);
        fprintf(stderr,
            "Warning: Failed to send welcome message. %s.\n", strerror(errno));
    } else {
        print_date_time_header(stdout);
        printf("Welcome message sent to %s.\n", connection_str);
    }
    int i = 0, bytes_recvd = recv(new_socket, inbuf, 1, 0);
    if (bytes_recvd == 0) {
        return EXIT_SUCCESS;
    }
    while (inbuf[i] != '\0') {
        i++;
        if (i > MAX_NAME_LEN + 1) {
            break;
        }
        bytes_recvd = recv(new_socket, inbuf + i, 1, 0);
    }
    if (bytes_recvd == -1) {
        print_date_time_header(stderr);
        fprintf(stderr, "Warning: Failed to receive user name. %s.\n",
            strerror(errno));
    } else {
        print_date_time_header(stdout);
        printf("Associated user name '%s' with %s.\n", inbuf, connection_str);
    }

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (client_sockets[i] == -1) {
            client_sockets[i] = new_socket;
            usernames[i] = strdup(inbuf);
            num_connections++;
            sprintf(outbuf, "User [%s] joined the chat room.", usernames[i]);
            broadcast_buffer(i, outbuf);
            break;
        }
    }
    return EXIT_SUCCESS;
}

void handle_client_socket(int index) {
    int port = 0;
    char ip[16];

    if (getpeername(client_sockets[index], (struct sockaddr*)&server_addr, &addrlen) == 0) {
        inet_ntop(AF_INET, &(server_addr.sin_addr), ip, 16);
        port = ntohs(server_addr.sin_port);
    } else {
        sprintf(ip, "0.0.0.0");
    }
    int i = 0,
        bytes_recvd = recv(client_sockets[index], inbuf, 1, 0);
    if (bytes_recvd == 0) {
        disconnect_client(index, ip, port);
        return;
    }
    while (inbuf[i] != '\0') {
        i++;
        if (i > MAX_MSG_LEN) {
            break;
        }
        bytes_recvd = recv(client_sockets[index], inbuf + i, 1, 0);
    }
    if (bytes_recvd == -1 && errno != EINTR) {
        print_date_time_header(stderr);
        fprintf(stderr,
            "Warning: Failed to receive incoming message from : [%s:%d]. %s.\n", ip, port, strerror(errno));
    } else {
        print_date_time_header(stdout);
        printf("Received from '%s' at [%s:%d]: %s\n", usernames[index],
            ip, port, inbuf);
        if (strcmp(inbuf, "bye") == 0) {
            disconnect_client(index, ip, port);
        } else {
            sprintf(outbuf, "[%s]: %s", usernames[index], inbuf);
            broadcast_buffer(index, outbuf);
        }
    }
}

int main(int argc, char *argv[]) {
    int retval = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port;
    if (!parse_int(argv[1], &port)) {
        return EXIT_FAILURE;
    }

    if (port < 1024 || port > 65535) {
        fprintf(stderr, "Error: port must be in range [1024, 65535].\n");
        return EXIT_FAILURE;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = catch_signal;
    if (sigaction(SIGINT, &action, NULL) == -1) {
        fprintf(stderr, "Error: Failed to register signal handler. %s.\n",
            strerror(errno));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        client_sockets[i] = -1;
        usernames[i] = NULL;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(
            server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) != 0) {
        fprintf(stderr, "Error: Failed to set socket options. %s.\n", strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }
    memset(&server_addr, 0, addrlen);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, addrlen) < 0) {
        fprintf(stderr, "Error: Failed to bind socket to port %d. %s.\n", port, strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }
    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        fprintf(stderr, 
                "Error: Failed to listen for incoming connections. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }
    printf("Chat is up and running on port %d.\nPress CTRL+C to exit.\n",
            port);
    fd_set sockset;
    int max_socket;
    while (running) {
        FD_ZERO(&sockset);
        FD_SET(server_socket, &sockset);
        max_socket = server_socket;

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (client_sockets[i] > -1) {
                FD_SET(client_sockets[i], &sockset);
            }
            if (client_sockets[i] > max_socket) {
                max_socket = client_sockets[i];
            }
        }

        if (select(max_socket + 1, &sockset, NULL, NULL, NULL) < 0 && errno != EINTR) {
            print_date_time_header(stderr);
            fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
            retval = EXIT_FAILURE;
            goto EXIT;
        }

        if (running && FD_ISSET(server_socket, (void *)(&socket))) {
            if (handle_server_socket() == EXIT_FAILURE) {
                retval = EXIT_FAILURE;
                goto EXIT;
            }
        }

        for (int i = 0; running && i < MAX_CONNECTIONS; i++) {
            if (client_sockets[i] > -1 && FD_ISSET(client_sockets[i], &sockset)) {
                handle_client_socket(i);
            }
        }
    }
EXIT:
    cleanup();
    printf("\n");
    print_date_time_header(stdout);
    printf("Shutting down.\n");
    return retval;
}


