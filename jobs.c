#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "jobs.h"
#include "utils.h"
#include "colors.h"
#include "pipe.h"

typedef struct {
    int pid;
    char cmd[MAX_CMD_LEN];
    int id;
} Job;

Job jobs[MAX_JOBS];
bool id_taken[MAX_JOBS];
int job_count = 0;

/**
 * @brief Return the status of the job with the given pid
 * @param pid The pid of the job
*/
int get_status(int pid) {
    int status;
    waitpid(pid, &status, WNOHANG);
    return status;
}

const char *status_to_string(int status) {
    if (WIFEXITED(status)) {
        return "Done";
    }else if (WIFSTOPPED(status)) {
        return "Stopped";
    }else {
        return "Running";
    }
}

/**
 * @brief Set the first free id in the list of jobs
*/
int set_first_free_id() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!id_taken[i]) {
            id_taken[i] = true;
            return i;
        }
    }
    fprintf(stderr,"Error: job list is full.\n");
    return -1;
}

/**
 * @brief Add a job to the list of jobs
 * @param pid The pid of the job
 * @param cmd The command of the job
 * @param status The status of the job
*/
int add_job(char **commande_args, bool has_pipe) {
    if (job_count < MAX_JOBS) {
        
        jobs[job_count].cmd[0] = '\0'; // Start with an empty string

        for (int i = 0; commande_args[i] != NULL; i++) {
            if (i > 0) {
                strcat(jobs[job_count].cmd, " "); // Add a space before each argument after the first
            }
            strcat(jobs[job_count].cmd, commande_args[i]);
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (has_pipe) {
                int n = nbPipes(commande_args);
                char ** tab_no_pipes = noPipe(commande_args, n);
                int ret = doPipe(tab_no_pipes, n + 1);
                free_tab(tab_no_pipes);
                return ret;
            }
            else{
                execvp(commande_args[0], commande_args);
                fprintf(stderr,"Error: command not found.\n");
                exit(0);
            }
        }
        else if (pid < 0) {
            fprintf(stderr,"Error: fork failed.\n");
            exit(0);
        }
        else {
            jobs[job_count].pid = pid;
            jobs[job_count].id = set_first_free_id();

            printf("[%d] %d\n", jobs[job_count].id, jobs[job_count].pid);

            job_count++;
            return 0;
        }
    } else {
        fprintf(stderr,"Error: job list is full.\n");
        return -1;
    }
}

/**
 * @brief Remove the job with the given pid
 * @param pid The pid of the job to remove
*/
void remove_job(int pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            id_taken[jobs[i].id] = false;
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
        
        char *job_id = malloc(number_length(jobs[i].id) + 2 + 1);   // 2 brackets + null terminator
        snprintf(job_id, number_length(jobs[i].id) + 2 + 1, "[%d]", jobs[i].id);
        color_switch(&job_id, red);

        printf("%s  %d  %s  %s\n", job_id, jobs[i].pid, status_to_string(get_status(jobs[i].pid)), jobs[i].cmd);
        
        free(job_id);
        if(get_status(jobs[i].pid) == 0){
            remove_job(jobs[i].pid);
        }
    }
}

/**
 * @brief Return the number of jobs
*/
int getNbJobs() {
    return job_count;
}