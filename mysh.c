#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "arraylist.h"

#define DEBUG 1

#ifndef DEBUG
#define DEBUG 0
#endif

#define BUFLENGTH 4096

int prev_process_exit_status;

int saved_stdout;

char* search_bare_names(char *command) {
    const char *directories[] = { "/usr/local/bin" , "/usr/bin", "/bin" };

    for (int i = 0; i < 3; i++) {
        
        size_t path_length = strlen(directories[i]) + strlen(command) + 1;
        char *path = (char *)malloc(path_length);

        strcpy(path, directories[i]);
        strcat(path, "/");
        strcat(path, command);

        int result = access(path, F_OK);

        if (result == 0) {
            return path;
        }

    }
    return NULL;
}

void execute_command(arraylist_t *token_stream) {
    
    int stream_size = token_stream->size;
    char *args[stream_size+1];
    int slash_flag = 0;
    
    for (int i = 0; i < stream_size; i++) {
        args[i] = strdup(al_get(token_stream, i));
    }

    for (int c = 0; c < strlen(args[0]); c++) {
        if (args[0][c] == '/') slash_flag = 1;
    }

    al_destroy(token_stream);

    if (DEBUG) printf("The command we try to run is: %s\n", args[0]);

    if (strcmp(args[0], "cd") == 0) {
        if (stream_size != 2) {
            fprintf(stderr, "cd - incorrect number of arguments\n");
            exit(EXIT_FAILURE);
        }
        if (chdir(args[1]) != 0) {
            perror("chdir");
            exit(1);
        }
        exit(0);
    } else if (strcmp(args[0], "pwd") == 0) {
        char buff[BUFLENGTH];
        if (getcwd(buff, BUFLENGTH) != NULL) {
            fprintf(stdout, "%s\n", buff);
            exit(0);
        }
        perror("pwd");
        exit(1);
    } else if (strcmp(args[0], "which") == 0) {
        if (search_bare_names(args[1]) != NULL) {
            fprintf(stdout, "%s\n", search_bare_names(args[1]));
            exit(0);
        }
        exit(1);
    } else if (strcmp(args[0], "exit") == 0) {
        exit(2);
    } else if (slash_flag) {
        args[stream_size] = NULL;
        execv(args[0], args);
        perror("exev");
        exit(1);
    } else {
        char *path = search_bare_names(args[0]);
        if (path != NULL) {
            args[stream_size] = NULL;
            execv(path, args);
            perror("execv");
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, "command - invalid input\n");
            exit(1);
        }
    } 
}

void execute(arraylist_t *token_stream) {
    int has_input_redirection = al_contains(token_stream, "<");
    char *new_infile = NULL;

    if (has_input_redirection != -1) {
        if (has_input_redirection <= token_stream->size - 2) {
            new_infile = strdup(al_get(token_stream, has_input_redirection + 1));
            al_remove(token_stream, has_input_redirection+1);
            al_remove(token_stream, has_input_redirection);
        } else {
            fprintf(stderr, "redirect - input redirect at end of line\n");
            al_destroy(token_stream);
            exit(EXIT_FAILURE);
        }
        int fd1 = open(new_infile, O_RDONLY);
        if (fd1 == -1) {
            perror("open");
            al_destroy(token_stream);
            exit(EXIT_FAILURE);
        } else {
            dup2(fd1, STDIN_FILENO);
        }
        if (close(fd1) == -1) {
            perror("close");
            al_destroy(token_stream);
            exit(EXIT_FAILURE);
        }
    }

    int has_output_redirection = al_contains(token_stream, ">");
    char *new_outfile = NULL;

    if (has_output_redirection != -1) {
        if (has_output_redirection <= token_stream->size - 2) {
            new_outfile = strdup(al_get(token_stream, has_output_redirection + 1));
            al_remove(token_stream, has_output_redirection+1);
            al_remove(token_stream, has_output_redirection);
        } else {
            fprintf(stderr, "redirect - output redirect at end of line\n");
            al_destroy(token_stream);
            exit(EXIT_FAILURE);
        }
        int fd2 = open(new_outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd2 == -1) {
            perror("open");
            al_destroy(token_stream);
            exit(EXIT_FAILURE);
        } else {
            dup2(fd2, STDOUT_FILENO);
        }
        if (close(fd2) == -1) {
            perror("close");
            al_destroy(token_stream);
            exit(EXIT_FAILURE);
        }
    }

    if (strcmp(al_get(token_stream, 0), "then") == 0) {
        if (prev_process_exit_status != 0) exit(EXIT_FAILURE);
        al_remove(token_stream, 0);
    } else if (strcmp(al_get(token_stream, 0), "else") == 0) {
        if (prev_process_exit_status != 1) exit(EXIT_FAILURE);
        al_remove(token_stream, 0);
    }

    execute_command(token_stream);

}

arraylist_t* tokenize_line(char *line) {
    char *ptr = line;
    int max_length = BUFLENGTH;
    char token[max_length];
    int word_index = 0;
    if (strcmp(line, "") == 0) {
        if (isatty(STDIN_FILENO)) return NULL;
        else exit(0);
    }

    arraylist_t *token_stream = al_init();

    while (*ptr != '\0') {
        if (word_index == max_length - 1){
            fprintf(stderr, "length - command entered is too long!\n");
            exit(EXIT_FAILURE);

        } else if ((*ptr == '<') || (*ptr == '>') || (*ptr == '|') || (isspace(*ptr))) {
            if (word_index > 0) {
                token[word_index] = '\0';
                al_append(token_stream, token);
            }
            
            if (!isspace(*ptr)) {
                char str[2];
                str[0] = *ptr;
                str[1] = '\0';
                al_append(token_stream, str); // Will put non-space symbols < > | * into arraylist
            }

            word_index = 0;
            memset(token, 0, max_length);

        } else {
            token[word_index] = *ptr;
            word_index++;
        }

        ptr++;
    }

    if (word_index > 0) {
        token[word_index+1] = '\0';
        al_append(token_stream, token);
        memset(token, 0, max_length);
    }
    
    return token_stream;
}

char* read_line(char *buffer) {

    int i = 0;
    char byte;
    while (i < BUFLENGTH - 1 && read(STDIN_FILENO, &byte, 1) > 0) {
        if (byte == '\n') {
            break;  // Exit loop when newline is encountered
        }
        buffer[i++] = byte;
    }
    buffer[i] = '\0';  // Null-terminate the string

    return buffer;
}

int run_child() {
    int exit_status = 2; // Exit value by default, ensures eventual end
    char buffer[BUFLENGTH];
    arraylist_t *token_stream = tokenize_line(read_line(buffer));
    if (token_stream == NULL) return 1;

    if (al_contains(token_stream, "|") != -1) {
        arraylist_t *pipe_tokens = al_sever(token_stream, al_contains(token_stream, "|"));

        int pipe_fd1[2];
        int pipe_fd2[2];

        if (pipe(pipe_fd1) == -1 || pipe(pipe_fd2) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Child process
            pid_t grandchild_pid = fork();

            if (grandchild_pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (grandchild_pid == 0) { // Grandchild process
                // Redirect standard input to the read end of the first pipe
                if (dup2(pipe_fd1[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }

                // Redirect standard output to the write end of the second pipe
                if (dup2(pipe_fd2[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }

                // Close unused ends of the pipes
                close(pipe_fd1[0]);
                close(pipe_fd1[1]);
                close(pipe_fd2[0]);
                close(pipe_fd2[1]);

                execute(pipe_tokens);
            } else { // Child process
                // Redirect standard output to the write end of the first pipe
                if (dup2(pipe_fd1[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }

                // Close unused ends of the pipes
                close(pipe_fd1[0]);
                close(pipe_fd2[0]);
                close(pipe_fd2[1]);

                execute(pipe_tokens);
            }
        } else { // Parent process
            // Close unused ends of the pipes
            close(pipe_fd1[1]);
            close(pipe_fd2[1]);

            // Redirect standard input to the read end of the second pipe
            if (dup2(pipe_fd2[0], STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            // Close unused ends of the pipes
            close(pipe_fd1[0]);
            close(pipe_fd2[0]);

            // Read from the second pipe and write to stdout
            dup2(saved_stdout, 1);
            close(saved_stdout);
            char buf2[BUFLENGTH];
            ssize_t bytes_read;
            while ((bytes_read = read(STDIN_FILENO, buf2, sizeof(buf2))) > 0) {
                write(STDOUT_FILENO, buf2, bytes_read);
            }

            if (bytes_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            // Wait for the child and grandchild processes to finish
            wait(NULL);
            wait(NULL);
        }
    } else {
        int pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE); // We exiting the whole program (parent) here because we should not be experiencing fork failures
        } else if (pid == 0) {
            execute(token_stream);
        } else {
            int status;
            if (wait(&status) == -1) {
                perror("wait");
                return 1;
            }
            if (WIFEXITED(status)) {
                exit_status = WEXITSTATUS(status);
            } else {
                fprintf(stderr, "Child process terminated abnormally\n");
                exit(EXIT_FAILURE);
            }
        }
        return exit_status;
    }
    
    return 2;

}

void change_stdin(char *infile) {
    int fd;
    fd = open(infile, O_RDONLY);
    
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    close (fd);
}

int main(int argc, char **argv) {
    //Checks proper usage
    if (argc == 2) change_stdin(argv[1]);
    else if (argc != 1) {
        fprintf(stderr, "usage - expected ./mysh OR ./mysh <filename>\n");
        exit(EXIT_FAILURE);
    }

    //
    saved_stdout = dup(1);

    prev_process_exit_status = -1;

    int mode = isatty(STDIN_FILENO);

    if (mode == 1) printf("Welcome to my shell!\n");

    while (1) {

        if (mode == 1) {
            printf("mysh> ");
            fflush(stdout);
        }

        prev_process_exit_status = run_child();

        if (prev_process_exit_status == 2) // Exit statuses: -1 error, 0 success, 1 failure, 2 terminate
            break;

    }

    if (mode == 1) printf("Exiting my shell...\n");

    return EXIT_SUCCESS;
}
