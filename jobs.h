#ifndef JOBS_H
#define JOBS_H

#define MAX_COMMANDS 100
#define MAX_JOBS 100
#define MAX_CMD_LEN 256
#define MAX_ARGS 100

int get_pid_by_id(int id);
int add_job(char **commande_args, bool has_pipe);
int remove_job(int pid);
int print_jobs();
int getNbJobs();
void verify_done_jobs();

#endif