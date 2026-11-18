#include "shell.h"
#include <fcntl.h>

void print_help(void)
{
    printf("Built-in commands:\n");
    printf("  cd [directory]  Change the current working directory\n");
    printf("  exit            Exit the shell\n");
    printf("  help            Show this help message\n");
    printf("  jobs            Show background jobs (added in a later feature)\n");
}

static int run_builtin(char **args, int *handled)
{
    *handled = 1;

    if (strcmp(args[0], "exit") == 0)
        return 1;

    if (strcmp(args[0], "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(args[0], "jobs") == 0) {
        puts("No job-control support has been added yet.");
        return 0;
    }

    if (strcmp(args[0], "cd") == 0) {
        const char *directory = args[1];

        if (directory == NULL)
            directory = getenv("HOME");

        if (directory == NULL) {
            fprintf(stderr, "cd: HOME is not set\n");
        } else if (chdir(directory) != 0) {
            perror("cd");
        }
        return 0;
    }

    *handled = 0;
    return 0;
}

static int apply_redirection(char **args)
{
    int i;

    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0) {
            int input = (strcmp(args[i], "<") == 0);
            char *filename = args[i + 1];
            int fd;

            if (filename == NULL) {
                fprintf(stderr, "myshell: missing file for redirection\n");
                return -1;
            }

            if (input)
                fd = open(filename, O_RDONLY);
            else
                fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd < 0) {
                perror(filename);
                return -1;
            }

            if (dup2(fd, input ? STDIN_FILENO : STDOUT_FILENO) < 0) {
                perror("dup2");
                close(fd);
                return -1;
            }

            close(fd);
            args[i] = NULL;
            return 0;
        }
    }

    return 0;
}

static void run_child(char **args)
{
    if (apply_redirection(args) < 0)
        _exit(1);

    if (args[0] == NULL)
        _exit(0);

    execvp(args[0], args);
    perror(args[0]);
    _exit(127);
}

static void wait_for(pid_t child)
{
    while (waitpid(child, NULL, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }
}

int execute(char **arglist)
{
    int handled;
    int i;
    int pipe_index = -1;
    int builtin_result;
    pid_t child;

    for (i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index < 0 &&
        (strcmp(arglist[0], "<") != 0) &&
        (strcmp(arglist[0], ">") != 0)) {
        builtin_result = run_builtin(arglist, &handled);
        if (handled)
            return builtin_result;
    }

    if (pipe_index >= 0) {
        int descriptors[2];
        pid_t left_child;
        pid_t right_child;

        if (arglist[pipe_index + 1] == NULL) {
            fprintf(stderr, "myshell: missing command after pipe\n");
            return 0;
        }

        arglist[pipe_index] = NULL;
        if (pipe(descriptors) < 0) {
            perror("pipe");
            return 0;
        }

        left_child = fork();
        if (left_child == 0) {
            close(descriptors[0]);
            dup2(descriptors[1], STDOUT_FILENO);
            close(descriptors[1]);
            run_child(arglist);
        }
        if (left_child < 0) {
            perror("fork");
            close(descriptors[0]);
            close(descriptors[1]);
            return 0;
        }

        right_child = fork();
        if (right_child == 0) {
            close(descriptors[1]);
            dup2(descriptors[0], STDIN_FILENO);
            close(descriptors[0]);
            run_child(&arglist[pipe_index + 1]);
        }
        close(descriptors[0]);
        close(descriptors[1]);

        if (right_child < 0)
            perror("fork");
        else
            wait_for(right_child);
        wait_for(left_child);
        return 0;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        return 0;
    }

    if (child == 0)
        run_child(arglist);

    wait_for(child);
    return 0;
}
