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


void Error(char* message) {
    fprintf(stderr, "%s%s%s\n", RED, message, RESET);
}

void increaseSize(char* Buffer, int* BuffSize) {
    *BuffSize += 1024;
    Buffer = realloc(Buffer, *BuffSize * sizeof(char));
    if (Buffer == NULL) Error("Allocation Error");
}

// read input line
void readInput(char** line, char** redirectionDest) {
    int lineSize = 1024; int redirectionDestSize = 1024;

    *line = malloc(lineSize * sizeof(char));

    int position = 0; int redirected = 0; int redirectionPosition = 0;
    if (*line == NULL) Error("Allocation Error");

    printf("gsh> "); fflush(stdout);
    while (1) {
        const char c = getchar();
        if (c == EOF || c == '\n') {
            if (redirected) (*redirectionDest)[redirectionPosition] = '\0';
            else (*line)[position] = '\0';
            return;
        } else if (c == '>' || redirected) {
            if (!redirected) {
                *redirectionDest = malloc(redirectionDestSize * sizeof(char));
            }

            (*redirectionDest)[redirectionPosition] = c;
            ++redirectionPosition; redirected++;

        } else if (!redirected) {
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
    if (tokens == NULL) Error("Allocation Error");

    char * token = strtok(line, TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        ++position;

        // Duplicated
        if (position >= BuffSize) {
            BuffSize += 1024;
            tokens = realloc(tokens, BuffSize * sizeof(char*));
            if (tokens == NULL) Error("Allocation Error");
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
        char* path = malloc(strlen(program) + strlen(PATH[i]) + 1);
        strcpy(path, PATH[i]);
        strcat(path, program);
        if (!access(path, X_OK)) return path;
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

void execute(char** args, char** PATH, int numOfPathDirs, char* redirectFile) {
    int tries = 1, offset = 0;
    if (!strcmp(args[0], "loop")) {
        tries = atoi(args[1]);
        offset = 2;
    }

    char* path = checkExecutable(PATH, numOfPathDirs, args[offset]);
    if (path != NULL) {
        for (int i = 0; i < tries; i++) {
            const int rc = fork();
            if (rc < 0) {
                Error("An error has occurred");
            } else if (rc == 0) {
                if (redirectFile != NULL) {
                    close(STDOUT_FILENO); close(STDERR_FILENO);
                    open(redirectFile, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
                }

                execv(path, args + offset);
            } else {
                wait(NULL);
            }
        }
        free(path);
    } else {
        Error("An error has occurred");
    }
}


int main(int argc, char **argv) {
    // set the PATH variable
    int numOfPathDirs = 1;
    char** PATH = malloc(numOfPathDirs * sizeof(char*));
    PATH[0] = malloc(6 * sizeof(char));
    strcpy(PATH[0], "/bin/");

    // implement batch file commands
    if (argc > 1) {
        if (argc != 2) {
            Error("An error has occurred"); terminate(1);
        }

        close(STDIN_FILENO);
        open(argv[1], O_RDONLY);
    }

    while (1) {
        // read the input
        char* line = NULL; char* redirectDest = NULL;
        readInput(&line, &redirectDest);

        int numOfArgs = 0;
        char** args = splitLine(line, &numOfArgs);

        char** redirectArgs = NULL; char* redirectFile = NULL;
        if (redirectDest != NULL) {
            int numOfredirectArgs;
            redirectArgs = splitLine(redirectDest, &numOfredirectArgs);
            if (numOfredirectArgs > 1) Error("Redirection Error");
            redirectFile = redirectArgs[1];
        }

        // run the `cd` built-in command
        if (!strcmp(args[0], "cd") && numOfArgs == 2) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                chdir(args[1]);
                printf("%s\n", getcwd(cwd, sizeof(cwd)));
            } else {
                Error("An error has occurred");
            }
        }
        // run `exit` built-in command
        else if (!strcmp(args[0], "exit")) {
            freeAll(PATH, numOfPathDirs); free(args); free(line);
            if (redirectFile != NULL) {
                free(redirectArgs); free(redirectDest);
            }

            if (redirectArgs != NULL) free(redirectArgs);
            exit(0);
        }
        // implement the `path` built-in command
        else if (!strcmp(args[0], "path")) {
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
        else {
            execute(args, PATH, numOfPathDirs, redirectFile);
        }
        free(args); free(line);
        if (redirectFile != NULL) {
            free(redirectArgs); free(redirectDest);
        }
    }
} 