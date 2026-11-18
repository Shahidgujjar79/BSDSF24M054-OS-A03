#include "shell.h"

int main(void)
{
    char *cmdline;
    char **arglist;
    int should_exit = 0;

    while (!should_exit && (cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        arglist = tokenize(cmdline);
        if (arglist != NULL) {
            should_exit = execute(arglist);
            free_args(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
