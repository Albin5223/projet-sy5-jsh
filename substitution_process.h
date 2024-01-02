#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#ifndef SUBSTITUTION_PROCESS_H
#define SUBSTITUTION_PROCESS_H

int nb_subs(char **command_args);
int ok_subs(char **command_args, int i);
char **get_substitution_process(char **command_args);
int execute_substitution_process(char **command_args, int nb_subs);
char **get_main_command(char **command_args);
int nb_args(char **command_args);
int exist_fic(char *path);

#endif