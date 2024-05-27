#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <dirent.h>

#define REQUEST_MAX_LEN 1024
#define LEN_DATA 40

#define ECHO_QUERY "ECHO"
#define QUIT_REQUEST "QUIT"
#define INFO_QUERY "INFO"
#define LIST_QUERY "LIST"
#define CD_QUERY "CD"

#define LINK_TO_FILE " --> "
#define LINK_TO_LINK " -->> "

#define UNKNOWN_QUERY "UNKNOWN"
#define EXIT_MESSAGE "Connection were shut down\n"
#define WRONG_PARAMETER "Theres no such command. Use LIST to see available.\n"

char request[REQUEST_MAX_LEN];
char cur_directory[REQUEST_MAX_LEN];
char list_of_files[REQUEST_MAX_LEN];

bool is_new_dir = false;

void list_files_in_directory(DIR *directory) {
    struct dirent *dir = NULL;
    struct stat sb;
    memset(list_of_files, '\0', strlen(list_of_files));
    strncat(list_of_files, cur_directory, strlen(cur_directory));
    strncat(list_of_files, "\n", strlen("\n"));
    while ((dir = readdir(directory))) {
        char fullpath[REQUEST_MAX_LEN];
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }
        sprintf(fullpath, "%s/%s", cur_directory, dir->d_name);
        if (lstat(fullpath, &sb) == -1) {
            continue;
        }
        if ((sb.st_mode & __S_IFMT) == __S_IFDIR) {
            strncat(list_of_files, dir->d_name, strlen(dir->d_name));
            strncat(list_of_files, "/", strlen("/"));
        } else if ((sb.st_mode & __S_IFMT) == __S_IFREG) {
            strncat(list_of_files, dir->d_name, strlen(dir->d_name));
        } else if ((sb.st_mode & __S_IFMT) == __S_IFLNK) {
            char target_path[REQUEST_MAX_LEN];
            ssize_t bytes = readlink(fullpath, target_path, sizeof(target_path) - 1);
            if (bytes != -1) {
                lstat(target_path, &sb);
                if ((sb.st_mode & __S_IFMT) == __S_IFDIR) {
                    strncat(list_of_files, dir->d_name, strlen(dir->d_name));
                    strncat(list_of_files, LINK_TO_FILE, strlen(LINK_TO_FILE));
                    strncat(list_of_files, target_path, strlen(target_path));
                } else if ((sb.st_mode & __S_IFMT) == __S_IFDIR) {
                    strncat(list_of_files, dir->d_name, strlen(dir->d_name));
                    strncat(list_of_files, LINK_TO_LINK, strlen(LINK_TO_LINK));
                    strncat(list_of_files, target_path, strlen(target_path));
                }
            }
        }
        strncat(list_of_files, "\n", strlen("\n"));
    }
    closedir(directory);
}

void get_server_info(const char *__filename, char **__buffer, size_t *__size) {
    FILE *file = fopen(__filename, "r");
    if (file == NULL) {
        printf("The file was not found. Check the data is correct.\n");
        return;
    }
    if (__buffer == NULL) {
        fclose(file);
        return;
    }
    fseek(file, 0, SEEK_END);
    *__size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *__buffer = (char *) realloc(*__buffer, *__size * sizeof(char));
    fread(*__buffer, sizeof(char), *__size, file);
    fclose(file);
}

void console_message(int fd_client, const char *cur_command, const char *arguments) {
    const time_t date_logging = time(NULL);
    struct tm *current_time = localtime(&date_logging);
    char str_date[LEN_DATA];
    strftime(str_date, 40, "%d.%m.%Y %H:%M:%S ", current_time);
    printf("%s COMMAND %s from [client-%d] with arguments = %s",
           str_date, cur_command, fd_client, arguments);
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        fprintf(stderr, "Dependencies: %s port, directory, serverinfo file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    strncpy(cur_directory, ".", strlen("."));
    DIR *d = opendir(argv[2]);
    if (d == NULL) {
        printf("The current directory could not be opened. "
               "Please check that the data is correct.");
        exit(EXIT_FAILURE);
    }
    getcwd(cur_directory, REQUEST_MAX_LEN);


    struct addrinfo addr_info = {0};
    struct addrinfo *result = NULL;

    addr_info.ai_next = NULL;
    addr_info.ai_canonname = NULL;
    addr_info.ai_addr = NULL;
    addr_info.ai_socktype = SOCK_STREAM;
    addr_info.ai_flags = AI_PASSIVE;
    addr_info.ai_protocol = 0;
    addr_info.ai_family = AF_INET;

    int soc = getaddrinfo(NULL, argv[1], &addr_info, &result);
    if (soc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(soc));
        exit(EXIT_FAILURE);
    }

    int socfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(socfd, result->ai_addr, result->ai_addrlen);
    printf("Server runs on port: %s\n", argv[1]);

    listen(socfd, SOMAXCONN);
    printf("Listening...\n");
    int fd_client = accept(socfd, result->ai_addr, &result->ai_addrlen);

    list_files_in_directory(d);

    const char *filename = argv[3];
    char *serverinfo = NULL;
    size_t file_size = 0;
    get_server_info(filename, &serverinfo, &file_size);
    write(fd_client, serverinfo, file_size);

    while (true) {
        ssize_t bytes_read = recv(fd_client, request, REQUEST_MAX_LEN, 0);
        size_t size_query = strlen(request);

        if (bytes_read > 0) {
            if (strncasecmp(request, ECHO_QUERY, strlen(ECHO_QUERY)) == 0) {
                console_message(fd_client, ECHO_QUERY, request);
                write(fd_client, request + strlen(ECHO_QUERY) + 1, size_query);
            } else if (strncasecmp(request, INFO_QUERY, strlen(INFO_QUERY)) == 0) {
                console_message(fd_client, INFO_QUERY, request);
                write(fd_client, serverinfo, file_size);
            } else if (strncasecmp(request, CD_QUERY, strlen(CD_QUERY)) == 0) {
                console_message(fd_client, CD_QUERY, request);
                request[size_query - 1] = '\0';
                if ((d = opendir(request + 3)) == NULL) {
                    write(fd_client, "", 1);
                } else {
                    strncpy(cur_directory, request + 3, size_query);
                    chdir(cur_directory);
                    getcwd(cur_directory, REQUEST_MAX_LEN);
                    d = opendir(cur_directory);
                    request[size_query - 1] = '\n';
                    write(fd_client, request + 3, size_query - 3);
                    is_new_dir = true;
                }
            } else if (strncasecmp(request, LIST_QUERY, strlen(LIST_QUERY)) == 0) {
                console_message(fd_client, LIST_QUERY, request);
                if (is_new_dir == true) {
                    list_files_in_directory(d);
                    is_new_dir = false;
                }
                write(fd_client, list_of_files, strlen(list_of_files));
            } else if (strncasecmp(request, QUIT_REQUEST, strlen(QUIT_REQUEST)) == 0) {
                console_message(fd_client, QUIT_REQUEST, request);
                write(fd_client, EXIT_MESSAGE, strlen(EXIT_MESSAGE));
            } else {
                console_message(fd_client, UNKNOWN_QUERY, request);
                write(fd_client, WRONG_PARAMETER, strlen(WRONG_PARAMETER));
            }
        } else if (bytes_read == 0) {
            printf("Client has closed connection\n");
            break;
        } else {
            fprintf(stderr, "Error while reading data\n");
        }
    }

    free(serverinfo);

    close(fd_client);
    close(socfd);

    return 0;
}
