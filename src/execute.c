#include "shell.h"

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

int execute(char **arglist)
{
    int handled;
    int builtin_result = run_builtin(arglist, &handled);
    pid_t child;

    if (handled)
        return builtin_result;

    child = fork();
    if (child < 0) {
        perror("fork");
        return 0;
    }

    if (child == 0) {
        execvp(arglist[0], arglist);
        perror(arglist[0]);
        _exit(127);
    }

    while (waitpid(child, NULL, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }

    return 0;
}
