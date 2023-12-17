#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#include "redirection.h"
#include "colors.h"
#include "utils.h"
#include "pipe.h"
#include "jobs.h"
#include "kill.h"

#include "internalCommand.h"


/**
 * Represents the maximum size of shell_path
*/
#define TRONCATURE_SHELL 30

/**
 * Global variable for dup
*/

int dup_stdin;
int dup_stdout;
int dup_stderr;


/**
 * @brief Return the path with the signe (like '$' or '>'), and color the path (without the signe)
*/
char *path_shell(char *signe, enum color job, enum color path){

    char *jobs = malloc(sizeof(char)*(2+number_length(getNbJobs())+1));  // Allocating the string for the jobs (2 brackets + number of jobs + null terminator)
    sprintf(jobs,"[%d]",getNbJobs());  // Adding the number of jobs to the string
    char *pwd = execute_pwd();  // Getting the path

    truncate_string(&pwd, TRONCATURE_SHELL-strlen(jobs)-strlen(signe));  // Truncating the path if it's too long

    color_switch(&jobs, job);   // Coloring the jobs
    color_switch(&pwd, path);   // Coloring the path

    char *prompt = calloc(sizeof(char),strlen(pwd)+strlen(jobs)+strlen(signe)+1);  // Allocating the prompt
    if (prompt == NULL) {
        fprintf(stderr,"Error in 'path_shell' : couldn't realloc...\n");
        exit(1);
    }

    strcat(prompt, jobs);  // Adding the jobs
    strcat(prompt, pwd);   // Adding the path
    strcat(prompt, signe); // Adding the signe

    free(pwd);  // Freeing the path
    free(jobs); // Freeing the jobs

    return prompt;
}

int main(int argc, char const *argv[]){
    
    char *input;
    
    char *init = execute_pwd();
    change_precedent(init); // We initialize the precedent to the current directory
    free(init);

    dup_stdin = dup(0);
    dup_stdout = dup(1);
    dup_stderr = dup(2);
    
    using_history();
    rl_outstream = stderr;

    while(1){

        char *path = path_shell("$ ", green, blue);
        
        input = readline(path);
        verify_done_jobs(); // We verify if there are jobs that are done, and we remove them from the list of jobs
        add_history(input);
        if(input == NULL){
            exit(getLastRetrunCode());
        }
        free(path);

        remove_last_spaces(&input); // Removing the last spaces only after verifying that the input is not empty
        if(strlen(input) == 0){    // If the input is empty after removing the last spaces, we continue
            continue;
        }

        char **commande_args;
        commande_args = get_tab_of_commande(input);
        if(commande_args == NULL){
            continue;
        }

        
        int last = execute_commande(commande_args);
        setLastRetrunCode(last);
        

        //On remet en place les dup

        dup2(dup_stdin,0);
        dup2(dup_stdout,1);
        dup2(dup_stderr,2);
        
        free(commande_args);
        free(input);
    }

    clear_history();
    
    
    return 0;
}
