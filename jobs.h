#ifndef JOBS_H
#define JOBS_H

#define MAX_COMMANDS 100
#define MAX_JOBS 256
#define MAX_SUB_CMD_LEN 256
#define MAX_SUBCOMMANDS 100
#define MAX_CHILD 100

enum status {RUNNING, DONE, STOPPED, KILLED, DETACHED, ALREADY_STOPPED};

int get_pid_by_id(int id);
int *getAllId();
int add_job(char **commande_args);
int remove_job(int pid);
int print_all_jobs(bool printChild);
int getNbJobs();
int print_job_with_pid(int pid ,bool printChild,int std);
void verify_done_jobs();
int execute_commande(char **commande_args);
int fg_id(int id);
int add_job_command(char **commande_args, bool is_background);
int execute_pipe_lancher(char **commande_args, bool is_background);

#endif