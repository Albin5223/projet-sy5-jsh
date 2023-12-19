#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>

#include "jobs.h"

/**
 * @brief says if the signal is valid
*/
bool is_valid_signal(int signal) {
    return signal > 0 && signal < NSIG;
}

/**
 * @brief send a signal to a process
 * @param pid the pid of the process
 * @param signal the signal to send
*/
int send_signal_to_pid(int pid, int signal) {
    if (!is_valid_signal(signal)) {
        printf("Error: invalid signal.\n");
        return 1;
    }
    int k = kill(pid, signal);
    return k;
}

/**
 * @brief kill a process with SIGTERM
 * @param pid the pid of the process
*/
int kill_one(int pid) {
    return send_signal_to_pid(pid, SIGTERM);
}

/**
 * @brief send a signal to a group of process
 * @param pgid the pgid of the group
 * @param signal the signal to send
*/
int send_signal_to_group(int pgid, int signal) {
    if (!is_valid_signal(signal)) {
        printf("Error: invalid signal.\n");
        return 1;
    }
    int k = killpg(pgid, signal);
    return k;
}

/**
 * @brief kill a group of process with SIGKILL
 * @param pgid the pgid of the group
*/
int kill_group(int pgid){
    return send_signal_to_group(pgid, SIGKILL);
}

/**
 * @brief send a signal to a job
 * @param id the id of the job
 * @param signal the signal to send
*/
int send_signal_to_id(int id, int signal) {
    int pid = get_pid_by_id(id);
    if (pid == -1) {
        printf("Error: job not found.\n");
        return 1;
    }
    int pgid = getpgid(get_pid_by_id(id));
    if (pgid == -1) {
        return send_signal_to_pid(get_pid_by_id(id), signal);
    }
    else {
        return send_signal_to_group(pgid, signal);
    }
}

/**
 * @brief kill a job with SIGTERM
 * @param id the id of the job
*/
int kill_id(int id) {
    return send_signal_to_id(id, SIGTERM);
}