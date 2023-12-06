#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>

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
    char cmd[MAX_CMD_LEN];
    int id;
} Job;

Job jobs[MAX_JOBS];
bool id_taken[MAX_JOBS];
int job_count = 0;

Command *split_commands_for_jobs(char **commande_args) {
    Command *commands = malloc(sizeof(Command) * MAX_COMMANDS);
    int i = 0;
    int j = 0;
    int k = 0;
    commands[0].cmd = malloc(sizeof(char*) * MAX_ARGS);
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
            commands[j].cmd = malloc(sizeof(char*) * MAX_ARGS);
            commands[j].is_background = false;
        } else {
            commands[j].cmd[k] = tmp;
            k++;
        }
        i++;
    }
    commands[j+1].cmd = NULL;
    return commands;
}


void free_splited_commands(char ***commands) {
    for (int i = 0; commands[i] != NULL; i++) {
        free(commands[i]);
    }
    free(commands);
}

int get_pid_by_id(int id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].id == id) {
            return jobs[i].pid;
        }
    }
    return -1;
}

/**
 * @brief Return the status of the job with the given pid
 * @param pid The pid of the job
*/
int update_status(int pid) {
    int status;
    int result = waitpid(pid, &status, WNOHANG);
    if (result == 0) {
        // Child is still running
        return -1;
    }
    else {
        // Child is done
        return status;
    }
}

const char *status_to_string(int status) {
    if (status == -1) {
        return "Running";
    } else if (status == -2) {
        return "Error";
    } else if (WIFEXITED(status)) {
        return "Done";
    } else if (WIFSTOPPED(status)) {
        return "Stopped";
    } else if (WIFCONTINUED(status)) {
        return "Continued";  
    } else if (WIFSIGNALED(status)) {
        return "Terminated by signal";
    } else {
        return "Unknown";
    }
}

void print_job(Job job) {
    char *job_id = malloc(number_length(job.id) + 2 + 1);   // 2 brackets + null terminator
    snprintf(job_id, number_length(job.id) + 2 + 1, "[%d]", job.id);
    color_switch(&job_id, red);
    printf("%s  %d  %s  %s\n", job_id, job.pid, status_to_string(update_status(job.pid)), job.cmd);
    free(job_id);
}

void print_job_old_status(Job job, int old_status){
    char *job_id = malloc(number_length(job.id) + 2 + 1);   // 2 brackets + null terminator
    snprintf(job_id, number_length(job.id) + 2 + 1, "[%d]", job.id);
    color_switch(&job_id, red);
    printf("%s  %d  %s  %s\n", job_id, job.pid, status_to_string(old_status), job.cmd);
    free(job_id);
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

int add_job_command(char **commande_args, bool is_background, bool has_pipe) {
    jobs[job_count].cmd[0] = '\0'; // Start with an empty string

    for (int i = 0; commande_args[i] != NULL; i++) {
        if (i > 0) {
            strcat(jobs[job_count].cmd, " "); // Add a space before each argument after the first
        }
        strcat(jobs[job_count].cmd, commande_args[i]);
    }

    pid_t pid = fork();
    if (pid == 0) { // Child process
        int descripteur_sortie_standart = -1;
        int descripteur_sortie_erreur = -1;

        int copie_sortie_standart = dup(STDOUT_FILENO);
        int copie_sortie_erreur = dup(STDERR_FILENO);
        /*
        * Mise en place des descripteurs en cas de redirections
        * On sait que la dernière redirection a la priorité, donc on a juste besoin de 2 descripteur, le premier pour la redirection pour la sortie standart
        * et le deuxième pour la redirection dans la sortie erreur
        * 
        * Dans cette condition, si il y a la présence d'une ou plusieurs redirection, on va affecter les descripteurs correspondant puis excécuter la commande
        * avec la suite du code
        */
        if(isRedirection(commande_args) != -1){
            int *fd = getDescriptorOfRedirection(commande_args);
            if(fd[0] != -1){
                descripteur_sortie_standart = fd[0];
            }
            if(fd[1] != -1){
                descripteur_sortie_erreur = fd[1];
            }

            if(isRedirectionStandart(commande_args) != -1 && fd[0] == -1){
                free(fd);
                return 1;
            }
            if(isRedirectionErreur(commande_args) != -1 && fd[1] == -1){
                free(fd);
                return 1;
            }

            //On récupère la commande cad : comm > fic, 
            commande_args = getCommandeOfRedirection(commande_args);
            free(fd);
        }

        if (is_background) { // If the command is run in the background
            setpgid(0, 0); // Set the process group ID to the process ID
        }
        if (has_pipe) { // If the command has a pipe, we need to do the pipe
            int n = nbPipes(commande_args);
            char ** tab_no_pipes = noPipe(commande_args, n);
            int ret = doPipe(tab_no_pipes, n + 1);
            free_tab(tab_no_pipes);
            return ret;
        }
        else{   // If the command does not have a pipe, we can just execute it
            execvp(commande_args[0], commande_args);
            fprintf(stderr,"Error: command not found.\n");
            exit(0);
        }

        /*
        *Si les descripteurs sont difféérents de -1 alors c'est qu'il y a un fichier qui est ouvert
        */
        if(descripteur_sortie_erreur >0){
            close(descripteur_sortie_erreur);
            dup2(copie_sortie_erreur,2);
        }
        if(descripteur_sortie_standart >0){
            close(descripteur_sortie_standart);
            dup2(copie_sortie_standart,1);
        }
    }
    else if (pid < 0) { // Error
        fprintf(stderr,"Error: fork failed.\n");
        exit(0);
    }
    else {  // Parent process
        if (!is_background) { // If the command is not run in the background
            int status;
            waitpid(pid, &status, 0); // Wait for the child process to finish
            return status;
        }
        else{
            jobs[job_count].pid = pid;
            jobs[job_count].id = set_first_free_id();
            printf("[%d] %d\n", jobs[job_count].id, jobs[job_count].pid);
            job_count++;
        }        
    }
    return 0;
}

int add_job(char **commande_args, bool has_pipe) {
    Command *commands = split_commands_for_jobs(commande_args);
    for (int i = 0; commands[i].cmd != NULL; i++) {
        int status = add_job_command(commands[i].cmd, commands[i].is_background, has_pipe);
        if (status != 0) {
            return status;
        }
    }
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
int print_jobs() {
    int i = 0;
    while (i < job_count){
        print_job(jobs[i]);
        if(update_status(jobs[i].pid) != -1){  // If the job is not running anymore, remove it from the list
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
        int old_status = update_status(jobs[i].pid);
        if(old_status != -1){  // If the job is not running anymore, print it and remove it from the list
            print_job_old_status(jobs[i],old_status);
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