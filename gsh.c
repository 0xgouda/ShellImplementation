#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

// read input line
char* readInput(int* checkEOF) {
    char *lineBuffer = NULL;
    size_t lineBufferSize = 0;
    printf("gsh> "); fflush(stdout);
    *checkEOF = getline(&lineBuffer, &lineBufferSize, stdin);
    return lineBuffer;
}

// splits the line into differnet words
char** splitString(char* line, int num) {
    char** command = malloc(sizeof(char *) * (num + 1));
    int i = 0; const unsigned long len = strlen(line);

    int cnt = 0;
    while (cnt < num) {
        while (i < len && isspace(*(line + i))) ++i;
        int j = i;
        while (j < len && !isspace(*(line + j))) ++j;

        char tmp = *(line + j);
        *(line + j) = '\0';

        command[cnt] = malloc(sizeof(line + i) + 1);
        strcpy(command[cnt], line + i);

        *(line + j) = tmp;
        i = j; ++i; cnt++;
    }
    command[num] = NULL;
    return command;
}

// search the executable in the PATH variable
char* checkExecutable(char** PATH, const int len, char* program) {
    for (int i = 0; i < len; i++) {
        char* path = malloc(strlen(program) + strlen(PATH[i]) + 1);
        strcpy(path, PATH[i]);
        strcat(path, program);
        if (!access(path, X_OK)) return path;
        free(path);
    }
    return NULL;
}

// parse the line to get the number of words, check if redirection is used
int parseLine(char* line, char** redirectString) {
    int num = 0, i = 0;
    const unsigned long len = strlen(line);
    while (i < len) {
        while (i < len && isspace(*(line + i))) ++i;
        while (i < len && !isspace(*(line + i))) {
            if (*(line + i) == '>') {
                *redirectString = line + i + 1;
                *(line + i) = '\0';
                return *(line + i - 1) == ' ' ? num : ++num;
            }
            ++i;
        }
        ++num;
    }
    return --num;
}

void freeAll(char** var, const int len) {
    for (int i = 0; i < len; i++) {
        free(var[i]);
    }
    free(var);
}

void raiseError() {
    printf("An error has occurred\n");
}

int main(int argc, char **argv) {
    // set the PATH variable
    char** PATH = malloc(1 * sizeof(char*));
    PATH[0] = malloc(6 * sizeof(char));
    strcpy(PATH[0], "/bin/"); int pathLen = 1;

    // implement batch file commands
    if (argc > 1) {
        if (argc != 2) {
            raiseError(); exit(1);
        }

        close(STDIN_FILENO);
        open(argv[1], O_RDONLY);
    }

    while (1) {
        // read the input
        int checkEOF;
        char* line = readInput(&checkEOF);
        if (checkEOF == -1) exit(0);
        // parse input
        char* redirectString = NULL; // get the redirected to string
        const int numOfArgs = parseLine(line, &redirectString);
        if (numOfArgs < 1) {
            raiseError(); continue;
        }


        char** redirectFile = NULL;
        if (redirectString != NULL) {
            char** check = NULL;
            if (parseLine(redirectString, check) > 1 || check != NULL) {
                raiseError();
                continue;
            }
            redirectFile = splitString(redirectString, 1);
        }

        char** command = splitString(line, numOfArgs);
        // run the `cd` built-in command
        if (!strcmp(command[0], "cd") && numOfArgs == 2) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                chdir(command[1]);
            } else {
                raiseError();
            }
        }
        // run `exit` built-in command
        else if (!strcmp(command[0], "exit")) {
            freeAll(PATH, pathLen); free(line); freeAll(command, numOfArgs);
            exit(0);
        }
        // implement the `path` built-in command
        else if (!strcmp(command[0], "path")) {
            freeAll(PATH, pathLen);
            PATH = malloc((numOfArgs - 1) * sizeof(char*));
            for (int i = 1; i < numOfArgs; i++) {
                PATH[i - 1] = malloc(strlen(command[i]) + 1);
                strcpy(PATH[i - 1], command[i]);
            }
            pathLen = numOfArgs - 1;
        }
        // run executable commands
        else {
            int tries = 1, offset = 0;
            if (!strcmp(command[0], "loop")) {
                tries = atoi(command[1]);
                offset = 2;
            }

            char* path = checkExecutable(PATH, pathLen, command[offset]);
            if (path != NULL) {
                for (int i = 0; i < tries; i++) {
                    const int rc = fork();
                    if (rc < 0) {
                        raiseError();
                    } else if (rc == 0) {
                        if (redirectFile != NULL) {
                            close(STDOUT_FILENO); close(STDERR_FILENO);
                            open(*redirectFile, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
                        }

                        execv(path, command + offset);
                    } else {
                        wait(NULL);
                    }
                }
                free(path);
            } else {
                raiseError();
            }
        }
        free(line); freeAll(command, numOfArgs);
        if (redirectFile != NULL) freeAll(redirectFile, 1);
    }
} 