#include "shell.h"

static char *history[HISTORY_SIZE];
static int history_count;
static int history_next;

char *read_cmd(const char *prompt, FILE *fp)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    printf("%s", prompt);
    fflush(stdout);

    length = getline(&line, &capacity, fp);
    if (length < 0) {
        free(line);
        return NULL;
    }

    if (length > 0 && line[length - 1] == '\n')
        line[length - 1] = '\0';

    return line;
}

void add_history(const char *command)
{
    char *copy = strdup(command);

    if (copy == NULL) {
        perror("strdup");
        return;
    }

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
