#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

void dirwlk(char* directoryName, bool options[]) {
    struct dirent** namelist = NULL;
    char* fullName = NULL;
    int n = -1;
    int dirLen = strlen(directoryName);
    struct stat stats;
    if (options[3]) {
        n = scandir(directoryName, &namelist, 0, alphasort);
    } else {
        n = scandir(directoryName, &namelist, 0, 0);
    }
    if (n < 0) {
        perror(directoryName);
        return;
    }
    if(n <= 2) {
        free(namelist);
        return;
    }
    else {
        for(int i = 0; i < n; i++) {
            if(strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
                continue;
            }
            fullName = calloc(512,sizeof(char));
            strcpy(fullName,directoryName);
            strcat(fullName,"/");
            strcat(fullName, namelist[i]->d_name);
            if(strlen(fullName) >= 512)
                return;
            if(lstat(fullName,&stats) == -1) {
                perror("lstat fail");
                return;
            }
            if(     (namelist[i]->d_type == DT_LNK && options[0]) ||
                    (namelist[i]->d_type == DT_REG && options[2]) ||
                    (namelist[i]->d_type == DT_DIR && options[1])
            ) {
                printf("%s\n", fullName);
            }
            if(namelist[i]->d_type == DT_DIR && namelist[i]->d_name[0] != '.') {
                dirwlk(fullName, options);
            }
            free(namelist[i]);
            free(fullName);
        }
        free(namelist);
        return;
    }
}

int main(int argc, char* argv[]) {
    bool options[] = {0,0,0,0};
    int curOpt;
    DIR* directory;
    char* directoryName = NULL;
    while((curOpt = getopt(argc,argv,"ldfs")) != -1) {
        switch(curOpt) {
            case 'l':                               //symbolic
                options[0] = true;
                break;
            case 'd':                               //folders
                options[1] = true;
                break;
            case 'f':                               //files
                options[2] = true;
                break;
            case 's':                               //sorted
                options[3] = true;
                break;
            default:
                break;
        }
    }

    if((argc > 1) && (argv[1][0] == '/')) {
        directoryName = calloc(strlen(argv[1])+1,1);
        strcpy(directoryName,argv[1]);
    } else if (argv[argc-1][0] == '/') {
        directoryName = calloc(strlen(argv[argc-1])+1,1);
        strcpy(directoryName,argv[argc-1]);
    } else if(directoryName == NULL) {
        directoryName = calloc(sizeof(char), 3);
        strcpy(directoryName, "./\0");
    }
    if(!options[0] && !options[1] && !options[2]) {
        options[0] = options[1] = options[2] = true;
    }
    dirwlk(directoryName, options);

    return 0;
}
