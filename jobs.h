#ifndef JOBS_H
#define JOBS_H

#define MAX_JOBS 100
#define MAX_CMD_LEN 256

void add_job(int pid, const char* cmd, const char* status);
void remove_job(int pid);
void print_jobs();
int getNbJobs();

#endif