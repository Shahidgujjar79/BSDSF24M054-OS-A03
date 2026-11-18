#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define PROMPT "FCIT> "

char *read_cmd(const char *prompt, FILE *fp);
char **tokenize(char *cmdline);
void free_args(char **arglist);
int execute(char **arglist);
void print_help(void);

#endif
