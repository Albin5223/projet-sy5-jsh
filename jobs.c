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
#include "internalCommand.h"
#include "kill.h"

typedef struct {
    char **cmd;
    bool is_background;
} Command;

typedef struct {
    int pid;
    char cmd[MAX_SUB_CMD_LEN];
    int id;
    enum status status;
    int exit_code;
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
        case KILLED:
            return "Killed";
        case DETACHED:
            return "Detached";
        default:
            return "Unknown";
    }
}

void print_job(Job job, int std){
    char *job_id = malloc(number_length(job.id) + 2 + 1);   // 2 brackets + null terminator
    snprintf(job_id, number_length(job.id) + 2 + 1, "[%d]", job.id);
    //color_switch(&job_id, red); // Les couleurs ne font pas passer les tests... dommage
    dprintf(std,"%s  %d  %s  %s\n", job_id, job.pid, status_to_string(job.status), job.cmd);
    free(job_id);
}

int print_job_with_pid(int pid, int std){
    for(int i = 0; i < job_count; i++){
        if(jobs[i].pid == pid){
            print_job(jobs[i], STDERR_FILENO);
            return 0;
        }
    }
    dprintf(2,"Error: job with pid %d not found.\n",pid);
    return 1;
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

int get_id_with_pid(int pid){
    for(int i = 0; i < job_count; i++){
        if(jobs[i].pid == pid){
            return jobs[i].id;
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
    else if(result == -1){
        printf("Error: job [%d] not found.\n",get_id_with_pid(pid));
        return -1;
    }
    else {
        // Child is done
        if (WIFEXITED(status)) {
            jobs[get_position_with_pid(pid)].status = DONE;
            jobs[get_position_with_pid(pid)].exit_code = WEXITSTATUS(status); // Store the exit status
            return DONE;
        }
        else if (WIFSIGNALED(status)) {
            jobs[get_position_with_pid(pid)].status = KILLED;
            jobs[get_position_with_pid(pid)].exit_code = WTERMSIG(status); // Store the termination signal
            return KILLED;
        }
        else if (WIFSTOPPED(status)) {
            jobs[get_position_with_pid(pid)].status = STOPPED;
            jobs[get_position_with_pid(pid)].exit_code = WSTOPSIG(status); // Store the termination signal
            return STOPPED;
        }
        else if (WIFCONTINUED(status)) {
            jobs[get_position_with_pid(pid)].status = RUNNING;
            print_job_with_pid(pid,2);
            return RUNNING;
        }
        else {
            jobs[get_position_with_pid(pid)].status = RUNNING;
            return RUNNING;
        }
    }
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
    dprintf(STDERR_FILENO,"Error: job list is full.\n");
    return -1;
}

int add_to_tab_of_jobs(int pid, char **commande_args){
    jobs[job_count].cmd[0] = '\0'; // Start with an empty string
    for (int i = 0; commande_args[i] != NULL; i++) {
        if (i > 0) {
            strcat(jobs[job_count].cmd, " "); // Add a space before each argument after the first
        }
        strcat(jobs[job_count].cmd, commande_args[i]);
    }
    
    job_count++;
    jobs[job_count-1].status = RUNNING;
    jobs[job_count-1].pid = pid;
    jobs[job_count-1].id = set_first_free_id();
    return 0;
}

int add_job_command(char **commande_args, bool is_background, bool has_pipe) {

    if(isInternalCommand(commande_args) && !is_background){
        return executeInternalCommand(commande_args);
    }

    pid_t pid = fork();
    if (pid == 0) { // Child process
        setpgid(0, 0); // Set the process group ID to the process ID


        int descripteur_sortie_standart = -1;
        int descripteur_sortie_erreur = -1;
        int descripteur_entree = -1; 
        
        /*
        * Mise en place des descripteurs en cas de redirections
        * On sait que la dernière redirection a la priorité, donc on a juste besoin de 2 descripteur, le premier pour la redirection pour la sortie standart
        * et le deuxième pour la redirection dans la sortie erreur
        * 
        * Dans cette condition, si il y a la présence d'une ou plusieurs redirection, on va affecter les descripteurs correspondant puis excécuter la commande
        * avec la suite du code
        */
       if(isRedirectionEntree(commande_args)!= -1 && !has_pipe){
            descripteur_entree = getFichierEntree(commande_args);
            if(descripteur_entree == -1){
                dprintf(STDERR_FILENO,"bash: %d: %s.\n", getFichierEntree(commande_args), strerror(errno));
                exit(1);
            }
            dup2(descripteur_entree,0);
            commande_args = getCommandeWithoutRedirectionEntree(commande_args);
        }

        if(isRedirection(commande_args) != -1 && !has_pipe){
            int *fd = getDescriptorOfRedirection(commande_args);
            if(fd[0] != -1){
                descripteur_sortie_standart = fd[0];
            }
            if(fd[1] != -1){
                descripteur_sortie_erreur = fd[1];
            }

            if(isRedirectionStandart(commande_args) != -1 && fd[0] == -1){
                free(fd);
                dprintf(STDERR_FILENO,"bash: sortie: %s\n", "cannot overwrite existing file");
                exit(1);
            }
            if(isRedirectionErreur(commande_args) != -1 && fd[1] == -1){
                free(fd);
                exit(1);
            }

            //On récupère la commande cad : comm > fic, 
            commande_args = getCommandeOfRedirection(commande_args);
            free(fd);
        }
        // If the command does not have a pipe, we can just execute it
        execvp(commande_args[0], commande_args);
        const char *error = strerror(errno);
        dprintf(STDERR_FILENO,"bash: %s: %s.\n", commande_args[0], error);
        //free_tab(commande_args);
        exit(1);    // If execvp returns, there was an error
    }
    else if (pid < 0) { // Error
        dprintf(STDERR_FILENO,"Error: fork failed.\n");
        exit(1);
    }
    else {  // Parent process
        if(add_to_tab_of_jobs(pid, commande_args) != 0){    // If the job could not be added, return 1
            return 1;
        }

        if (!is_background) { // If the command is not run in the background
            while (1){
                update_status(pid);
                if(jobs[get_position_with_pid(pid)].status == DONE || jobs[get_position_with_pid(pid)].status == KILLED){
                    int exit_code = jobs[get_position_with_pid(pid)].exit_code; // Get the exit code of the job
                    remove_job(pid);
                    return exit_code;
                }
                else if(jobs[get_position_with_pid(pid)].status == STOPPED){
                    print_job_with_pid(pid, STDERR_FILENO);
                    return 0;
                }
                else if(jobs[get_position_with_pid(pid)].status == RUNNING){
                    continue;
                }
                else{
                    dprintf(STDERR_FILENO,"Error: unknown status.\n");
                    return 1;
                }
            }
        }
        else{
            return print_job_with_pid(pid, STDERR_FILENO);
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

    int j = 0;
    for(int i = 0; commands[i].cmd[0] != NULL; i++){
        free(commands[i].cmd);
        j++;
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
            jobs[i].exit_code = -1;
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
            dprintf(STDERR_FILENO,"Error: job [%d] not found.\n",jobs[i].id);
            return -1;
        }
        print_job(jobs[i], STDOUT_FILENO);
        if(jobs[i].status == DONE || jobs[i].status == KILLED){  // If the job has exited, or killed, remove it from the list
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
            dprintf(STDERR_FILENO,"Error: job [%d] not found.\n",jobs[i].id);
            return;
        }
        if(jobs[i].status == DONE || jobs[i].status == KILLED){  // If the job has exited, or killed, print it and remove it from the list
            print_job(jobs[i], STDERR_FILENO);
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



int execute_commande(char **commande_args){
    int n = nbPipes(commande_args); //On regarde le nombre de pipes
    if(n != 0){ //Si il y a des pipes alors on execute la fonction add_jobs avec true en parametre
        return add_job(commande_args,true);
    }
    else{ //sinon on execute la fonction add_jobs avec false en parametre
        return add_job(commande_args,false);
    }
}