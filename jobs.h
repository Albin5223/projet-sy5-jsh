#ifndef JOBS_H
#define JOBS_H

#define MAX_COMMANDS 100
#define MAX_JOBS 256
#define MAX_SUB_CMD_LEN 256
#define MAX_SUBCOMMANDS 100

enum status {RUNNING, DONE, STOPPED, KILLED, DETACHED};

int get_pid_by_id(int id);
int add_job(char **commande_args, bool has_pipe);
int remove_job(int pid);
int print_all_jobs();
int getNbJobs();
void verify_done_jobs();
int execute_commande(char **commande_args);

#endif