#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "jobs.h"

typedef struct {
    int pid;
    char cmd[MAX_CMD_LEN];
    char status[8]; // Running, Done, Stopped
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;

/**
 * @brief Add a job to the list of jobs
 * @param pid The pid of the job
 * @param cmd The command of the job
 * @param status The status of the job
*/
void add_job(int pid, const char* cmd, const char* status) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].cmd, cmd, MAX_CMD_LEN);
        strncpy(jobs[job_count].status, status, 8);
        job_count++;
    } else {
        printf("Error: job list is full.\n");
    }
}

/**
 * @brief Remove the job with the given pid
 * @param pid The pid of the job to remove
*/
void remove_job(int pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            // Shift all elements down to fill the gap
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            return;
        }
    }
    printf("Error: job not found.\n");
}

/**
 * @brief Print the list of jobs
*/
void print_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d  %s  %s\n", i + 1, jobs[i].pid, jobs[i].status, jobs[i].cmd);
    }
}

/**
 * @brief Return the number of jobs
*/
int getNbJobs() {
    return job_count;
}