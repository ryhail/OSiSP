#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define REQUEST_MAX_LEN 1024
#define QUIT_REQUEST "QUIT"

char BUF[REQUEST_MAX_LEN];
char request[REQUEST_MAX_LEN];


bool is_end_session() {
    if (strncasecmp(request, QUIT_REQUEST, strlen(QUIT_REQUEST)) == 0) {
        return true;
    }
    return false;
}

void proccesing_request(int client_fd, bool* flag_end) {
    write(client_fd, request, REQUEST_MAX_LEN);
    *flag_end = is_end_session();
    ssize_t bytes_read = recv(client_fd, request, REQUEST_MAX_LEN, 0);
    request[bytes_read] = '\0';
    if (bytes_read > 0) {
        fputs(request, stdout);
    } else {
        fprintf(stderr, "Server shut the connection");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
}

void request_from_file(int client_fd, bool* flag_end) {
    FILE* file = fopen(request + 1, "r");
    if (file == NULL) {
        printf("File not found.\n");
        return;
    }
    while(!feof(file)) {
        fgets(request, REQUEST_MAX_LEN, file);
        printf("> %s", request);
        proccesing_request(client_fd, flag_end);
        if (*flag_end == true) {
            return;
        }
    }
}


int main(int argc, char* argv[])
{

    if (argc != 2) {
        fprintf(stderr, "Dependency: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct addrinfo addr_info = {0};
    struct addrinfo* result = NULL;

    addr_info.ai_family = AF_INET;
    addr_info.ai_socktype = SOCK_STREAM;
    addr_info.ai_protocol = 0;
    addr_info.ai_flags = 0;

    int s = getaddrinfo(NULL, argv[1], &addr_info, &result);

    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    int fd_client = socket(addr_info.ai_family, addr_info.ai_socktype, addr_info.ai_addrlen);
    int flag_connect = connect(fd_client, result->ai_addr, result->ai_addrlen);

    if (flag_connect == -1) {
        perror("server error");
        exit(EXIT_FAILURE);
    }

    read(fd_client, BUF, REQUEST_MAX_LEN);
    printf("%s", BUF);

    bool flag_end = false;
    while(flag_end == false) {
        printf("> ");
        fgets(request, REQUEST_MAX_LEN, stdin);
        if (request[0] == '@') {
            request[strlen(request) - 1] = '\0';
            request_from_file(fd_client, &flag_end);
        } else {
            proccesing_request(fd_client, &flag_end);
        }
    }

    close(fd_client);
    return 0;
}
