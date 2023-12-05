#ifndef JOBS_H
#define JOBS_H

#define MAX_COMMANDS 100
#define MAX_JOBS 100
#define MAX_CMD_LEN 256
#define MAX_ARGS 100

char*** split_commands_for_jobs(char **commande_args);
void free_splited_commands(char ***commands);
int get_pid_by_id(int id);
int add_job(char **commande_args, bool isBackground, bool has_pipe);
void remove_job(int pid);
void print_jobs();
int getNbJobs();
void verify_done_jobs();

#endif