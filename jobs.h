#ifndef JOBS_H
#define JOBS_H

#define MAX_JOBS 100
#define MAX_CMD_LEN 256

int get_pid_by_id(int id);
int add_job(char **commande_args, bool has_pipe);
void remove_job(int pid);
void print_jobs();
int getNbJobs();

#endif