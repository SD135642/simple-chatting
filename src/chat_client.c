#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"


int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];



void argc_check(int argc, char *prog_name) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s, <server IP>, <port>\n", prog_name);
        exit(EXIT_FAILURE);
    }
}

void argv_check(char **argv, struct sockaddr_in *server_ip) {
    int ip_conversion = inet_pton(AF_INET, argv[1], &server_ip->sin_addr);
    if (ip_conversion < 0) {
        fprintf(stderr, "Failed to convert IP address '%s'. %s.\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
        
    } else if(ip_conversion == 0) {
        fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
        exit(EXIT_FAILURE);
    } else {
        for (int i = 0; i < strlen(argv[2]); i++) {
            char temp_char = argv[2][i];
            if (temp_char < '0' || temp_char > '9') {
                fprintf(stderr, "Error: Invalid port entry '%s'.\n", argv[2]);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void get_username(void) {
    int process = 1;
    while (process == 1) {
        printf("Enter your desired username: ");
        char temp[MAX_NAME_LEN + 2];
        if (fgets(temp, MAX_NAME_LEN + 2, stdin) == NULL) {
            fprintf(stderr, "Error: Could not read from stdin. %s.\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        size_t name_length = strlen(temp) - 1;
        if (temp[name_length] != '\n') {
            printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
            char temp_char = getchar();
            while (temp_char != '\n') {
                temp_char = getchar();
            }
        } else if (name_length != 0) {
            for (int i = 0; temp[i] != '\0'; i++) {
                if (temp[i] != '\n') {
                    username[i] = temp[i];
                } else {
                    username[i] = '\0';
                }
            }
            printf("Hello, %s. Let's try to connect to the server.\n", username);
            process = 0;
        }
    }
}

void close_client_socket(int client_socket) {
    if (fcntl(client_socket, F_GETFD) != -1) {
        close(client_socket);
    }
}

void drop_chars(void) {
    outbuf[0] = 0;
    ssize_t read_chars = 1;
    while (outbuf[0] != '\n' && read_chars != 0) {
        read_chars = read(STDIN_FILENO, outbuf, 1);
        if (read_chars == -1) {
            fprintf(stderr, "Error: read() failed. %s.\n", strerror(errno));
            close_client_socket(client_socket);
            exit(EXIT_FAILURE);
        }
    }
}

void process_input(void) {
    int i = 0;

    while (1) {
        ssize_t read_chars = read(STDIN_FILENO, outbuf + i, 1);
        if (read_chars == -1) {
            fprintf(stderr, "Error: read() failed. %s.\n", strerror(errno));
            close_client_socket(client_socket);
            exit(EXIT_FAILURE);
        } else if (read_chars == 0) {
            outbuf[i] = '\0';
            ssize_t send_status;
            if ((send_status = send(client_socket, outbuf, i + 1, 0)) == -1) { 
                fprintf(stderr, "Error: failed to send messages to server. %s.\n", strerror(errno));
                close_client_socket(client_socket);
                exit(EXIT_FAILURE);
            }
            break;
        } else if (outbuf[i] == '\n') {
            outbuf[i] = '\0';
            ssize_t send_status;
            if ((send_status = send(client_socket, outbuf, i + 1, 0)) == -1) { 
                fprintf(stderr, "Error: failed to send messages to server. %s.\n", strerror(errno));
                close_client_socket(client_socket);
                exit(EXIT_FAILURE);
            }
            if (strcmp(outbuf, "bye\n") == 0) {
                printf("Goodbye.\n");
                close_client_socket(client_socket);
                exit(EXIT_FAILURE);
            }
            break;
        }
        i++;
        if (i == MAX_MSG_LEN) {
            fprintf(stderr, "Sorry, limit your message to 1 line of at most %d characters.\n",
                        MAX_MSG_LEN);
            drop_chars();
            break;
        }
    }
}

void process_socket_input(char inbuf[], int more_bytes_recvd) {
    if (strcmp(inbuf, "bye\n") == 0) {
        printf("\nServer initiated shutdown.\n");
        close_client_socket(client_socket);
        exit(EXIT_FAILURE);
    } else {
        printf("\n%s\n\n", inbuf);
    }
}

void handle_client_socket(struct sockaddr_in server_ip, socklen_t ip_length) {
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    server_ip.sin_family = AF_INET;
    if (connect(client_socket, (struct sockaddr *)&server_ip, ip_length) < 0) {
        fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
        close_client_socket(client_socket);
        exit(EXIT_FAILURE);
    }
    int bytes_recvd;
    if ((bytes_recvd = recv(client_socket, inbuf, BUFLEN, 0)) <= 0) {
        if (bytes_recvd < 0) {
            fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
        } else if (bytes_recvd == 0) {
            fprintf(stderr, "Error: Connection was closed.\n");
        }
        close_client_socket(client_socket);
        exit(EXIT_FAILURE);
    }
    inbuf[bytes_recvd] = '\0';
    printf("\n%s\n\n", inbuf);
    ssize_t send_status;
    if ((send_status = send(client_socket, username, sizeof(username) + 1, 0)) == -1) {
        fprintf(stderr, "Error: Failed to send messages to server. %s.\n", strerror(errno));
        close_client_socket(client_socket);
        exit(EXIT_FAILURE);
    }

    fd_set rd;
    while (1) {
        FD_ZERO(&rd);
        FD_SET(client_socket, &rd);
        FD_SET(STDIN_FILENO, &rd);
        int largest_fd = (client_socket > STDIN_FILENO) ? client_socket : STDIN_FILENO;
        
        int decr_num = select(largest_fd + 1, &rd, NULL, NULL, NULL);

        if (decr_num == -1) {
            fprintf(stderr, "Error: Select failed. %s.\n", strerror(errno));
            close_client_socket(client_socket);
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(STDIN_FILENO, &rd)) {
            if (!feof(stdin)) {
                process_input();
            }               
            if (!feof(stdin)) {
                process_input();
            } else if (FD_ISSET(client_socket, &rd)) {
                int more_bytes_recvd;
                if ((more_bytes_recvd = recv(client_socket, inbuf, BUFLEN, 0)) <= 0) {
                    if (more_bytes_recvd < 0 && errno != EINTR) {
                        printf("Warning: Failed to receive incoming message.\n");
                    } else if (more_bytes_recvd < 0) {
                        fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
                        close_client_socket(client_socket);
                        exit(EXIT_FAILURE);
                    } else if (more_bytes_recvd == 0) {
                        fprintf(stderr, "\nConnection to server has been lost.\n");
                        close_client_socket(client_socket);
                        exit(EXIT_FAILURE);               
                    }
                }
                else {
                    inbuf[more_bytes_recvd] = '\0';
                    process_socket_input(inbuf, more_bytes_recvd);
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    argc_check(argc, argv[0]);
    struct sockaddr_in server_ip;
    socklen_t ip_length = sizeof(server_ip);
    memset(&server_ip, 0, ip_length);
    argv_check(argv, &server_ip);
    get_username();
    handle_client_socket(server_ip, ip_length);
    return EXIT_SUCCESS;
}
