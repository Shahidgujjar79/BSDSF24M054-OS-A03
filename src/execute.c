#include "shell.h"
#include <fcntl.h>

#define MAX_JOBS 64

typedef struct {
    pid_t pid;
    char command[256];
} Job;

static Job job_list[MAX_JOBS];
static int job_count;

static void reap_jobs(void)
{
    int i = 0;
    int status;
    pid_t result;

    while (i < job_count) {
        result = waitpid(job_list[i].pid, &status, WNOHANG);
        if (result == job_list[i].pid) {
            printf("[%d] Done  %s\n", (int)job_list[i].pid, job_list[i].command);
            job_list[i] = job_list[job_count - 1];
            job_count--;
        } else {
            i++;
        }
    }
}

static void show_jobs(void)
{
    int i;

    reap_jobs();
    for (i = 0; i < job_count; i++)
        printf("[%d] Running  %s\n", (int)job_list[i].pid, job_list[i].command);
    if (job_count == 0)
        puts("No background jobs.");
}

static void remember_job(pid_t pid, char **args)
{
    int i;
    size_t used = 0;

    if (job_count == MAX_JOBS) {
        fprintf(stderr, "myshell: job table is full\n");
        return;
    }

    job_list[job_count].pid = pid;
    job_list[job_count].command[0] = '\0';

    for (i = 0; args[i] != NULL && used < sizeof(job_list[job_count].command) - 1; i++) {
        int written = snprintf(job_list[job_count].command + used,
                               sizeof(job_list[job_count].command) - used,
                               "%s%s", i ? " " : "", args[i]);
        if (written < 0)
            break;
        used += (size_t)written;
    }
    job_count++;
    printf("[%d] %d\n", job_count, (int)pid);
}

void print_help(void)
{
    puts("Built-in commands:");
    puts("  cd [directory]  Change the current working directory");
    puts("  exit            Exit the shell");
    puts("  help            Show this help message");
    puts("  jobs            List background jobs");
    puts("  history         Show recent commands");
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
        show_jobs();
        return 0;
    }

    if (strcmp(args[0], "cd") == 0) {
        const char *directory = args[1];
        if (directory == NULL)
            directory = getenv("HOME");
        if (directory == NULL)
            fprintf(stderr, "cd: HOME is not set\n");
        else if (chdir(directory) != 0)
            perror("cd");
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
            fd = input ? open(filename, O_RDONLY)
                       : open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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

static int execute_one(char **args, int background)
{
    int handled;
    int i;
    int pipe_index = -1;
    pid_t child;

    if (args[0] == NULL)
        return 0;

    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index < 0) {
        int result = run_builtin(args, &handled);
        if (handled)
            return result;
    }

    if (pipe_index >= 0) {
        int descriptors[2];
        pid_t left_child, right_child;

        if (args[pipe_index + 1] == NULL) {
            fprintf(stderr, "myshell: missing command after pipe\n");
            return 0;
        }
        args[pipe_index] = NULL;
        if (pipe(descriptors) < 0) {
            perror("pipe");
            return 0;
        }

        left_child = fork();
        if (left_child == 0) {
            close(descriptors[0]);
            dup2(descriptors[1], STDOUT_FILENO);
            close(descriptors[1]);
            run_child(args);
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
            run_child(&args[pipe_index + 1]);
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
        run_child(args);

    if (background)
        remember_job(child, args);
    else
        wait_for(child);

    return 0;
}


static int condition_succeeded(char **condition)
{
    pid_t child;
    int status;

    child = fork();
    if (child < 0) {
        perror("fork");
        return 0;
    }
    if (child == 0) {
        execvp(condition[0], condition);
        perror(condition[0]);
        _exit(127);
    }

    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return 0;
        }
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static int execute_if(char **args)
{
    int then_index = -1;
    int else_index = -1;
    int fi_index = -1;
    int i;
    int condition_ok;

    for (i = 1; args[i] != NULL; i++) {
        if (strcmp(args[i], "then") == 0 && then_index < 0)
            then_index = i;
        else if (strcmp(args[i], "else") == 0 && then_index >= 0 && else_index < 0)
            else_index = i;
        else if (strcmp(args[i], "fi") == 0) {
            fi_index = i;
            break;
        }
    }

    if (then_index < 2 || fi_index < 0 ||
        (else_index >= 0 && else_index > fi_index)) {
        fprintf(stderr, "myshell: syntax: if CONDITION then COMMAND [else COMMAND] fi\\n");
        return 0;
    }

    args[then_index] = NULL;
    condition_ok = condition_succeeded(&args[1]);

    if (else_index >= 0)
        args[else_index] = NULL;
    args[fi_index] = NULL;

    if (condition_ok)
        return execute(&args[then_index + 1]);
    if (else_index >= 0)
        return execute(&args[else_index + 1]);

    return 0;
}

int execute(char **arglist)
{
    int start = 0;
    int i;
    int should_exit = 0;

    reap_jobs();

    if (arglist[0] != NULL && strcmp(arglist[0], "if") == 0)
        return execute_if(arglist);

    for (i = 0; ; i++) {
        if (arglist[i] == NULL || strcmp(arglist[i], ";") == 0 ||
            strcmp(arglist[i], "&") == 0) {
            int background = (arglist[i] != NULL &&
                              strcmp(arglist[i], "&") == 0);
            char *separator = arglist[i];

            arglist[i] = NULL;
            if (i > start)
                should_exit = execute_one(&arglist[start], background);
            if (separator == NULL || should_exit)
                break;
            arglist[i] = separator;
            start = i + 1;
        }
    }

    return should_exit;
}
