#include "shell.h"

int main(void)
{
    char *cmdline;
    char **arglist;
    int should_exit = 0;

    while (!should_exit && (cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        if (cmdline[0] == '!' && cmdline[1] != '\0') {
            char *end;
            long number = strtol(cmdline + 1, &end, 10);
            char *saved;

            if (*end != '\0' || number < 1 ||
                (saved = get_history_command((int)number)) == NULL) {
                fprintf(stderr, "myshell: history entry not found\n");
                free(cmdline);
                continue;
            }

            printf("%s\n", saved);
            free(cmdline);
            cmdline = strdup(saved);
            if (cmdline == NULL) {
                perror("strdup");
                break;
            }
        }

        if (cmdline[0] != '\0')
            shell_add_history(cmdline);

        {
            char *expanded = expand_variables(cmdline);
            if (expanded == NULL) {
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = expanded;
        }

        arglist = tokenize(cmdline);
        if (arglist != NULL) {
            if (strcmp(arglist[0], "history") == 0)
                show_history();
            else
                should_exit = execute(arglist);

            free_args(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
