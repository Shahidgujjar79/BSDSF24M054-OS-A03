#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

static char *history[HISTORY_SIZE];
static int history_count;
static int history_next;

static const char *commands[] = {
    "cd", "exit", "help", "jobs", "history", NULL
};

static char *command_generator(const char *text, int state)
{
    static int index;
    size_t length = strlen(text);

    if (state == 0)
        index = 0;

    while (commands[index] != NULL) {
        const char *name = commands[index++];
        if (strncmp(name, text, length) == 0)
            return strdup(name);
    }

    return NULL;
}

static char **shell_completion(const char *text, int start, int end)
{
    (void)end;

    if (start == 0)
        return rl_completion_matches(text, command_generator);

    return NULL;
}

char *read_cmd(const char *prompt, FILE *fp)
{
    (void)fp;
    rl_attempted_completion_function = shell_completion;
    return readline(prompt);
}

void shell_add_history(const char *command)
{
    char *copy = strdup(command);

    if (copy == NULL) {
        perror("strdup");
        return;
    }

    add_history(command);
    free(history[history_next]);
    history[history_next] = copy;
    history_next = (history_next + 1) % HISTORY_SIZE;

    if (history_count < HISTORY_SIZE)
        history_count++;
}

void show_history(void)
{
    int i;
    int first = (history_next - history_count + HISTORY_SIZE) % HISTORY_SIZE;

    for (i = 0; i < history_count; i++) {
        int index = (first + i) % HISTORY_SIZE;
        printf("%d  %s\n", i + 1, history[index]);
    }
}

char *get_history_command(int number)
{
    int first;
    int index;

    if (number < 1 || number > history_count)
        return NULL;

    first = (history_next - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    index = (first + number - 1) % HISTORY_SIZE;
    return history[index];
}

char **tokenize(char *cmdline)
{
    char **args;
    char *saveptr = NULL;
    char *token;
    int count = 0;

    if (cmdline == NULL)
        return NULL;

    args = calloc(MAXARGS + 1, sizeof(char *));
    if (args == NULL) {
        perror("calloc");
        return NULL;
    }

    token = strtok_r(cmdline, " \t", &saveptr);
    while (token != NULL) {
        if (count == MAXARGS) {
            fprintf(stderr, "myshell: too many arguments (maximum %d)\n", MAXARGS);
            free_args(args);
            return NULL;
        }

        args[count] = strdup(token);
        if (args[count] == NULL) {
            perror("strdup");
            free_args(args);
            return NULL;
        }

        count++;
        token = strtok_r(NULL, " \t", &saveptr);
    }

    if (count == 0) {
        free(args);
        return NULL;
    }

    args[count] = NULL;
    return args;
}

void free_args(char **arglist)
{
    int i;

    if (arglist == NULL)
        return;

    for (i = 0; arglist[i] != NULL; i++)
        free(arglist[i]);

    free(arglist);
}
