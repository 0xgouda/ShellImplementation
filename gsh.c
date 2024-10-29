#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define TOK_DELIM " \n\t\r"
#define RED "\033[0;31m"
#define RESET "\e[0m"

/*
  TODO
  1. add $loop var to hold current loop number
  2. better structure of execute with backgrounded and backgroundedCheck
  3. remove duplicate frees
 */


void raiseError() {
    fprintf(stderr, "%s%s%s\n", RED, "An error has occurred", RESET);
}

void increaseSize(char* Buffer, int* BuffSize) {
    *BuffSize += 1024;
    Buffer = realloc(Buffer, *BuffSize * sizeof(char));
    if (Buffer == NULL) raiseError();
}

// read input line
void readInput(char** line, char** redirectionDest, int* backgrounded) {
    int lineSize = 1024; int redirectionDestSize = 1024;

    *line = malloc(lineSize * sizeof(char));

    int position = 0; int redirected = 0; int redirectionPosition = 0;
    if (*line == NULL) raiseError();

    while (1) {
        const char c = getchar();

        if (c == '&')
            (*backgrounded)++;

        if (c == EOF || c == '\n' || c == '&') {
            if (!position) {
                free(*line); *line = NULL;
            }

            if (redirected && redirectionPosition) (*redirectionDest)[redirectionPosition] = '\0';
            else if (position) (*line)[position] = '\0';
            return;
        } else if (redirected) {
            (*redirectionDest)[redirectionPosition] = c;
            ++redirectionPosition; redirected++;

        } else if (!redirected) {
            if (c == '>') {
                *redirectionDest = malloc(redirectionDestSize * sizeof(char));
                redirected++; continue;
            }

            (*line)[position] = c;
            ++position;
        }

        if (position >= lineSize) {
            increaseSize(*line, &lineSize);
        }

        if (redirectionPosition >= redirectionDestSize) {
            increaseSize(*redirectionDest, &redirectionDestSize);
        }
    }
}

// splits the line into different words
char** splitLine(char* line, int* numOfArgs) {
    int position = 0; int BuffSize = 1024;
    char** tokens = malloc(BuffSize * sizeof(char*));
    if (tokens == NULL) raiseError("Allocation raiseError");

    char * token = strtok(line, TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        ++position;

        // Duplicated
        if (position >= BuffSize) {
            BuffSize += 1024;
            tokens = realloc(tokens, BuffSize * sizeof(char*));
            if (tokens == NULL) raiseError("Allocation raiseError");
        }

        token = strtok(NULL, TOK_DELIM);
    }
    tokens[position] = NULL;
    *numOfArgs = position;

    return tokens;
}

int terminate(const int status) {
     exit(status);
}

// search the executable in the PATH variable
char* checkExecutable(char** PATH, const int len, char* program) {
    for (int i = 0; i < len; i++) {
        char* path = malloc(strlen(program) + strlen(PATH[i]) + 1024);
        // support relative paths
        if (PATH[i][0] != '/') {
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            strcpy(path, cwd); strcat(path, "/");
            strcat(path, PATH[i]);
        }
        else strcpy(path, PATH[i]);

        strcat(path, "/");
        strcat(path, program);
        if (!access(path, X_OK))
            return path;
        free(path);
    }
    return NULL;
}

void freeAll(char** var, const int len) {
    for (int i = 0; i < len; i++) {
        free(var[i]);
    }
    free(var);
}

void execute(char** args, char** PATH, int numOfPathDirs, char* redirectFile, int backgrounded, int backgroundedCheck) {
    int tries = 1, offset = 0;
    if (!strcmp(args[0], "loop")) {
        tries = atoi(args[1]);
        offset = 2;
    }

    char* path = checkExecutable(PATH, numOfPathDirs, args[offset]);
    if (path != NULL) {
        for (int i = 0; i < tries; i++) {
            const int pid = fork();
            if (pid < 0) {
                raiseError("An error has occurred");
            } else if (pid == 0) {
                if (redirectFile != NULL) {
                    close(STDOUT_FILENO); close(STDERR_FILENO);
                    open(redirectFile, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
                }

                execv(path, args + offset);
            } else {
                if (!backgroundedCheck) {
                    for (int j = 0; j < backgrounded + 1; j++) {
                        wait(NULL);
                    }
                }
            }
        }
        free(path);
    } else {
        raiseError();
    }
}


int main(int argc, char **argv) {
    // set the PATH variable
    int numOfPathDirs = 1;
    char** PATH = malloc(numOfPathDirs * sizeof(char*));
    PATH[0] = malloc(5 * sizeof(char));
    strcpy(PATH[0], "/bin");

    // implement batch file commands
    if (argc > 1) {
        if (argc != 2) {
            raiseError(); terminate(1);
        }

        close(STDIN_FILENO);
        open(argv[1], O_RDONLY);

        char c = getchar();
        if (c != EOF) {
            ungetc(c, stdin);
        } else {
            raiseError(); terminate(1);
        }
    }

    // number of & backgrounded processes
    int backgrounded = 0;
    while (1) {
        // print the prompt if no file is provided
        if (argc <= 1) {
            printf("gsh> ");
            fflush(stdout);
        }

        // read the input
        int backgroundedCheck = 0;
        char* line = NULL; char* redirectDest = NULL;
        readInput(&line, &redirectDest, &backgroundedCheck);
        backgrounded += backgroundedCheck;

        if (line == NULL) {
            if (redirectDest != NULL) {
                free(redirectDest);
                raiseError(); continue;
            }

            for (int i = 0; i < backgrounded; i++) {
                wait(NULL);
            }

            continue;
        }

        int numOfArgs = 0;
        char** args = splitLine(line, &numOfArgs);

        char** redirectArgs = NULL; char* redirectFile = NULL;
        if (redirectDest != NULL) {
            int numOfredirectArgs;
            redirectArgs = splitLine(redirectDest, &numOfredirectArgs);
            if (numOfredirectArgs >= 2 || redirectArgs[0] == NULL || args[0] == NULL) {
                free(args); free(line); free(redirectDest); free(redirectArgs);
                raiseError(); continue;
            }
            redirectFile = redirectArgs[0];
        }

        // run the `cd` built-in command
        if (numOfArgs && !strcmp(args[0], "cd")) {
            char cwd[1024];
            if (numOfArgs == 2 && getcwd(cwd, sizeof(cwd)) != NULL) {
                chdir(args[1]);
                printf("%s\n", getcwd(cwd, sizeof(cwd)));
            } else {
                raiseError();
            }
        }
        // run `exit` built-in command
        else if (numOfArgs == 1 && !strcmp(args[0], "exit")) {
            freeAll(PATH, numOfPathDirs); free(args); free(line);
            if (redirectFile != NULL) {
                free(redirectArgs); free(redirectDest);
            }

            if (redirectArgs != NULL) free(redirectArgs);
            exit(0);
        }
        // implement the `path` built-in command
        else if (numOfArgs && !strcmp(args[0], "path")) {
            freeAll(PATH, numOfPathDirs);
            PATH = malloc((numOfArgs - 1) * sizeof(char*));
            for (int i = 1; i < numOfArgs; i++) {
                PATH[i - 1] = malloc(strlen(args[i]) + 1);
                strcpy(PATH[i - 1], args[i]);
                printf("%s\n", PATH[i - 1]);
            }
            numOfPathDirs = numOfArgs - 1;
        }
        // run executable commands
        else if (numOfArgs) {
            execute(args, PATH, numOfPathDirs, redirectFile, backgrounded, backgroundedCheck);
        }

        free(line);
        if (redirectFile != NULL) {
            free(redirectArgs); free(redirectDest);
        }
    }
} 