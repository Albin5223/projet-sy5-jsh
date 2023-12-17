#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "jobs.h"
#include "utils.h"
#include "colors.h"
#include "pipe.h"
#include "redirection.h"

typedef struct {
    char **cmd;
    bool is_background;
} Command;

typedef struct {
    int pid;
    char cmd[MAX_SUB_CMD_LEN];
    int id;
    enum status status;
} Job;

Job jobs[MAX_JOBS];
bool id_taken[MAX_JOBS];
int job_count = 0;

const char *status_to_string(enum status status){
    switch (status) {
        case RUNNING:
            return "Running";
        case DONE:
            return "Done";
        case STOPPED:
            return "Stopped";
        case CONTINUED:
            return "Continued";
        case KILLED:
            return "Killed";
        case DETACHED:
            return "Detached";
        default:
            return "Unknown";
    }
}

/**
 * @brief Split the given command into multiple subcommands, separated by the & character (for background execution, but subcommands do not contain the & character)
 * @param commande_args The command to split
 * @return An array of commands
*/
Command *split_commands_for_jobs(char **commande_args) {
    // Count the number of "&" symbols
    int command_count = 0;  // Start from 1 to account for the first command
    bool last_was_ampersand = false;
    for (int i = 0; commande_args[i] != NULL; i++) {
        if (strcmp(commande_args[i], "&") == 0) {
            command_count++;
            last_was_ampersand = true;
        } else {
            last_was_ampersand = false;
        }
    }

    // If the last command wasn't followed by an "&", increment the command_count
    if (!last_was_ampersand) {
        command_count++;
    }

    // Allocate memory for commands based on the count
    Command *commands = malloc(sizeof(Command) * (command_count + 1));  // +1 for the NULL terminator
    int i = 0;
    int j = 0;
    int k = 0;
    commands[0].cmd = malloc(sizeof(char*) * MAX_SUBCOMMANDS);
    commands[0].is_background = false;
    while (1) {
        char *tmp = commande_args[i];
        if (tmp == NULL) {
            commands[j].cmd[k] = NULL;
            break;
        }
        if (strcmp(tmp, "&") == 0) {
            commands[j].cmd[k] = NULL;
            commands[j].is_background = true;
            j++;
            k = 0;
            commands[j].cmd = malloc(sizeof(char*) * MAX_SUBCOMMANDS);
            commands[j].is_background = false;
            commands[j].cmd[0] = NULL;  // Initialize the cmd of the new command to NULL
        } else {
            commands[j].cmd[k] = tmp;
            k++;
        }
        i++;
    }
    if (k != 0) {  // Only set cmd to NULL if a new command was started
        commands[j+1].cmd = malloc(sizeof(char*));  // Allocate memory for commands[j+1].cmd
        commands[j+1].cmd[0] = NULL;
    }
    return commands;
}


void free_splited_commands(char ***commands) {
    for (int i = 0; commands[i] != NULL; i++) {
        free(commands[i]);
    }
    free(commands);
}

int get_pid_by_id(int id){
    for(int i = 0; i < job_count; i++){
        if(jobs[i].id == id){
            return jobs[i].pid;
        }
    }
    return -1;
}

int get_position_with_pid(int pid){
    for(int i = 0; i < job_count; i++){
        if(jobs[i].pid == pid){
            return i;
        }
    }
    return -1;
}

/**
 * @brief Update the status of the job with the given pid
 * @param pid The pid of the job
*/
enum status update_status(int pid) {
    int status;
    int result = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (result == 0) {
        // Child is still running
        if(jobs[get_position_with_pid(pid)].status == STOPPED){ // If the job was stopped, return stopped and do not update the status
            return STOPPED; 
        } 
        else{   // Else, return running and update the status
            jobs[get_position_with_pid(pid)].status = RUNNING;
            return RUNNING;
        }   
    }
    else {
        // Child is done
        if (WIFEXITED(status)) {
            jobs[get_position_with_pid(pid)].status = DONE;
            return DONE;
        }
        else if (WIFSIGNALED(status)) {
            jobs[get_position_with_pid(pid)].status = KILLED;
            return KILLED;
        }
        else if (WIFSTOPPED(status)) {
            jobs[get_position_with_pid(pid)].status = STOPPED;
            return STOPPED;
        }
        else if (WIFCONTINUED(status)) {
            jobs[get_position_with_pid(pid)].status = CONTINUED;
            return CONTINUED;
        }
        else {
            // TODO : handle DETACHED status
            printf("Error: unknown status.\n");
            return -1;
        }
    }
}

void print_job(Job job){
    char *job_id = malloc(number_length(job.id) + 2 + 1);   // 2 brackets + null terminator
    snprintf(job_id, number_length(job.id) + 2 + 1, "[%d]", job.id);
    color_switch(&job_id, red);
    printf("%s  %d  %s  %s\n", job_id, job.pid, status_to_string(job.status), job.cmd);
    free(job_id);
}

void print_job_with_AND(Job job){
    char *job_id = malloc(number_length(job.id) + 2 + 1);   // 2 brackets + null terminator
    snprintf(job_id, number_length(job.id) + 2 + 1, "[%d]", job.id);
    color_switch(&job_id, red);
    printf("%s  %d  %s  %s %s\n", job_id, job.pid, status_to_string(job.status), job.cmd, "&");
    free(job_id);
}

void print_pid_job(Job job){
    char *job_id = malloc(number_length(job.id) + 2 + 1);   // 2 brackets + null terminator
    snprintf(job_id, number_length(job.id) + 2 + 1, "[%d]", job.id);
    color_switch(&job_id, red);
    printf("%s  %d\n", job_id, job.pid);
    free(job_id);
}

/**
 * @brief Set the first free id in the list of jobs
*/
int set_first_free_id() {
    id_taken[0] = true;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!id_taken[i]) {
            id_taken[i] = true;
            return i;
        }
    }
    fprintf(stderr,"Error: job list is full.\n");
    return -1;
}

int add_job_command(char **commande_args, bool is_background, bool has_pipe) {
    pid_t pid = fork();

    if (pid < 0) {  // Error
        fprintf(stderr,"Error: fork failed.\n");
        exit(1);
    }

    if (pid == 0) { // Child process
        if (is_background) { // If the command is run in the background
            setpgid(0, 0); // Set the process group ID to the process ID
        }

        int descripteur_entree_standart = -1;
        int descripteur_sortie_standart = -1;
        int descripteur_sortie_erreur = -1;

        if(isRedirection(commande_args) != -1 && !has_pipe){
            int *fd = getDescriptorOfRedirection(commande_args);
            if(fd[0] != -1){
                descripteur_entree_standart = fd[0];
            }
            if(fd[1] != -1){
                descripteur_sortie_standart = fd[1];
            }
            if(fd[2] != -1){
                descripteur_sortie_erreur = fd[2];
            }

            if(isRedirectionStandart(commande_args) != -1 && fd[1] == -1 && fd[0] == -1){
                free(fd);
                return 1;
            }
            if(isRedirectionErreur(commande_args) != -1 && fd[2] == -1){
                free(fd);
                return 1;
            }

            //On récupère la commande cad, on passe de : 'cmd > fic' à 'cmd'
            commande_args = getCommandeOfRedirection(commande_args);
            free(fd);
        }

        if (has_pipe) { // If the command has a pipe, we need to do the pipe
            int ret = makePipe(commande_args);
            exit(ret);
        }
        else{   // If the command does not have a pipe, we can just execute it
            if (descripteur_entree_standart > 0) {
                dup2(descripteur_entree_standart, 0);
            }
            if (descripteur_sortie_standart > 0) {
                dup2(descripteur_sortie_standart, 1);
            }
            if (descripteur_sortie_erreur > 0) {
                dup2(descripteur_sortie_erreur, 2);
            }
            
            execvp(commande_args[0], commande_args);
            const char *error = strerror(errno);
            fprintf(stderr,"bash: %s: %s.\n", commande_args[0], error);
            //free_tab(commande_args);
            exit(1);    // If execvp returns, there was an error
        }
    }       
    else {  // Parent process
        if (!is_background) { // If the command is not run in the background
            int status;
            waitpid(pid, &status, 0); // Wait for the child process to finish
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            else {
                fprintf(stderr,"Error: child process exited abnormally.\n");
                return 1;
            }
        }
        else{
            jobs[job_count].cmd[0] = '\0'; // Start with an empty string
            for (int i = 0; commande_args[i] != NULL; i++) {
                if (i > 0) {
                    strcat(jobs[job_count].cmd, " "); // Add a space before each argument after the first
                }
                strcat(jobs[job_count].cmd, commande_args[i]);
            }

            job_count++;

            jobs[job_count-1].pid = pid;
            jobs[job_count-1].id = set_first_free_id();
            if(update_status(pid) == -1){
                fprintf(stderr,"Error: job [%d] not found.\n",jobs[job_count].id);
                return 1;
            }
            print_pid_job(jobs[job_count-1]);
        }        
    }
    return 0;
}

int add_job(char **commande_args, bool has_pipe) {
    Command *commands = split_commands_for_jobs(commande_args);
    for (int i = 0; commands[i].cmd[0] != NULL; i++) {
        int status = add_job_command(commands[i].cmd, commands[i].is_background, has_pipe);
        if (status != 0) {
            int j = 0;
            for(int i = 0; commands[i].cmd[0] != NULL; i++){
                free(commands[i].cmd);
                j++;
            }
            free(commands[j].cmd);
            free(commands);
            return status;
        }
    }

    int j;
    for(j = 0; commands[j].cmd[0] != NULL; j++){
        free(commands[j].cmd);
    }
    free(commands[j].cmd);
    free(commands);

    return 0;
}

/**
 * @brief Remove the job with the given pid
 * @param pid The pid of the job to remove
*/
int remove_job(int pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            id_taken[jobs[i].id] = false;
            jobs[i].cmd[0] = '\0';
            jobs[i].pid = -1;
            jobs[i].id = -1;
            jobs[i].status = -1;
            // Shift all elements down to fill the gap
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            return 0;
        }
    }
    printf("Error: job not found.\n");
    return 1;
}

/**
 * @brief Print the list of jobs
*/
int print_all_jobs() {
    int i = 0;
    while (i < job_count){
        if(update_status(jobs[i].pid) == -1){
            fprintf(stderr,"Error: job [%d] not found.\n",jobs[i].id);
            return -1;
        }
        print_job_with_AND(jobs[i]);
        enum status status = jobs[i].status;
        if(status == DONE || status == KILLED){  // If the job has exited, or killed, remove it from the list
            if(remove_job(jobs[i].pid) != 0){
                return 1;
            }
        }
        else{   // If the job is still running, increment i
            i++;
        }
    }
    return 0;
}

/**
 * @brief Verify if any jobs are done and print them, then remove them from the list
*/
void verify_done_jobs() {
    int i = 0;
    while (i < job_count){
        if(update_status(jobs[i].pid) == -1){
            fprintf(stderr,"Error: job [%d] not found.\n",jobs[i].id);
            return;
        }
        enum status status = jobs[i].status;
        if(status == DONE || status == KILLED){  // If the job has exited, or killed, print it and remove it from the list
            print_job(jobs[i]);
            remove_job(jobs[i].pid);    // Do not increment i, since the next job will have the same index
        }
        else{   // If the job is still running, increment i
            i++;
        }
    }
}

/**
 * @brief Return the number of jobs
*/
int getNbJobs() {
    return job_count;
}