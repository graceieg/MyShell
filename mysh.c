#include <stdio.h>
#include <glob.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 100

// Built-in command functions
void cd_command(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "mysh: expected argument to \"cd\"\n");
    } else if (chdir(args[1]) != 0) {
        perror("mysh");
    }
}

void pwd_command(char **args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("mysh");
    }
}

void exit_command(char **args) {
    if (args[1]) {
        printf("Exiting my shell: ");
        for (int i = 1; args[i]; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
    } else {
        printf("Exiting my shell\n");
    }
    exit(0);  // Immediately exit the shell
}

void which_command(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "mysh: expected argument to \"which\"\n");
        return;
    }

    const char *paths[] = {"/usr/local/bin", "/usr/bin", "/bin"};
    for (int i = 0; i < 3; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", paths[i], args[1]);
        if (access(path, F_OK) == 0) {
            printf("%s\n", path);
            return;
        }
    }
    printf("mysh: %s: command not found\n", args[1]);
}

// Array of built-in command functions and names
void (*builtin_functions[])(char **) = {cd_command, pwd_command, exit_command, which_command};
const char *builtin_commands[] = {"cd", "pwd", "exit", "which"};

// Redirection handling
int handle_redirection(char **args) {
    int i = 0;
    char *input_file = NULL, *output_file = NULL;

    // Check for redirection symbols
    while (args[i]) {
        if (strcmp(args[i], "<") == 0) {
            input_file = args[i + 1];
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            output_file = args[i + 1];
            args[i] = NULL;
        }
        i++;
    }

    // Handle input redirection
    if (input_file) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0 || dup2(fd_in, STDIN_FILENO) < 0) {
            perror("mysh: input redirection failed");
            return 0;
        }
        close(fd_in);
    }

    // Handle output redirection
    if (output_file) {
        if (output_file == NULL) {
            fprintf(stderr, "mysh: missing file operand\n");
            return 0;  // Return failure for missing file operand
        }

        int fd_out = open(output_file, O_CREAT | O_TRUNC | O_WRONLY, 0640);
        if (fd_out < 0 || dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("mysh: output redirection failed");
            return 0;
        }
        close(fd_out);
    }

    return 1;  // Success
}

int execute_command(char **args) {
    pid_t pid = fork();

    if (pid == 0) {  // Child process
        fflush(stdout);  // Flush output before execution

        // Check if it's a built-in command
        for (int i = 0; i < sizeof(builtin_functions) / sizeof(builtin_functions[0]); i++) {
            if (strcmp(args[0], builtin_commands[i]) == 0) {
                builtin_functions[i](args);
                exit(0);
            }
        }

        // Handle redirection
        if (!handle_redirection(args)) {
            exit(1);
        }

        // Execute external command
        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(1);
        }
    } else if (pid < 0) {
        perror("Fork failed");
        return 0;
    } else {  // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for the child to finish

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            printf("mysh: Command failed with code %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("mysh: Command terminated by signal %d\n", WTERMSIG(status));
        }

        // Print a newline and ensure prompt appears correctly after redirection
        printf("\n");
        fflush(stdout);  // Ensure the prompt is flushed and printed on a new line
    }

    return 0;  // No prompt here, handled in main
}

// Expanding wildcards
int expand_wildcards(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strchr(args[i], '*')) {  // Check if wildcard is present
            glob_t glob_result;
            int ret = glob(args[i], 0, NULL, &glob_result);  // Perform wildcard expansion
            
            if (ret == 0) {
                // If glob matches, replace the argument with the matched files
                int j = 0;
                while (j < glob_result.gl_pathc) {
                    args[i + j] = strdup(glob_result.gl_pathv[j]);
                    j++;
                }
                args[i + j] = NULL;  // Null-terminate the argument list
            } else if (ret == GLOB_NOMATCH) {
                // If no match was found, keep the original argument
                args[i] = strdup(args[i]);
            } else {
                perror("glob");
                return 0;  // Return failure if glob fails
            }
            
            globfree(&glob_result);  // Free memory used by glob
        }
    }
    return 1;
}

// Handle piping
int handle_pipe(char **args) {
    int i = 0, j = 0, pipe_count = 0;
    char *commands[10][MAX_ARGS];  // Array to store each command in the pipeline
    int pipefd[2], prev_pipe_read = -1;

    // Split the command into individual pipeline stages
    while (args[i]) {
        if (strcmp(args[i], "|") == 0) {
            commands[pipe_count][j] = NULL;  // End of current command
            pipe_count++;
            j = 0;
        } else {
            commands[pipe_count][j++] = args[i];  // Add argument to current command
        }
        i++;
    }
    commands[pipe_count][j] = NULL;  // Null-terminate the last command

    // Execute each stage of the pipeline
    for (int k = 0; k <= pipe_count; k++) {
        if (k < pipe_count && pipe(pipefd) < 0) {  // Create a pipe for all but the last stage
            perror("pipe failed");
            return 0;
        }

        pid_t pid = fork();
        if (pid == 0) {  // Child process
            if (prev_pipe_read != -1) {
                // Set input from previous pipe's read end
                dup2(prev_pipe_read, STDIN_FILENO);
                close(prev_pipe_read);
            }

            if (k < pipe_count) {
                // Set output to current pipe's write end
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }

            close(pipefd[0]);

            if (execvp(commands[k][0], commands[k]) == -1) {
                perror("execvp failed");
                exit(1);
            }
        } else {  // Parent process
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            prev_pipe_read = pipefd[0];
            close(pipefd[1]);
            waitpid(pid, NULL, 0);  // Wait for the child process
        }
    }

    return 1;
}

// Main loop to read and execute commands
int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];

    // Display the welcome message
    printf("Welcome to my shell!\n");

    while (1) {
        printf("mysh> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;  // Exit on EOF
        }

        // Remove newline character at the end of input
        input[strcspn(input, "\n")] = 0;

        // Tokenize the input into arguments
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] == NULL) {
            continue;  // Empty input
        }

        // Handle built-in commands
        if (strcmp(args[0], "exit") == 0) {
            exit_command(args);
        } else if (strcmp(args[0], "cd") == 0) {
            cd_command(args);
        } else if (strcmp(args[0], "pwd") == 0) {
            pwd_command(args);
        } else if (strcmp(args[0], "which") == 0) {
            which_command(args);
        } else {
            // Handle pipes and wildcards
            if (strchr(input, '|')) {
                handle_pipe(args);
            } else {
                expand_wildcards(args);  // Handle wildcards
                execute_command(args);
            }
        }
    }
    return 0;
}
