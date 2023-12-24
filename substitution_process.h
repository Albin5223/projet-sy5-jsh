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
bool ok_subs(char **command_args, int i);
char **get_substitution_process(char **command_args, int nb);
int execute_substitution_process(char **command_args, int nb);
int subs_write_in_pipe(char ***tab, int *pipefd, char **final_cmd, int start);
char **get_main_command(char **command_args);

#endif