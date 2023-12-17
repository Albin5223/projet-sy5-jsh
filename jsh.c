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

/**
 * Represents the maximum size of the path
*/
#define MAX_PATH_SIZE 2048
/**
 * Represents the maximum size of shell_path
*/
#define TRONCATURE_SHELL 30

int is_running = 1;

/**
 * @brief Execute the command 'pwd' and return the path
*/
char *execute_pwd(){
    char *pwd = malloc(sizeof(char)*MAX_PATH_SIZE); // Allocating the path
    if (!pwd) return NULL;      // -> adapter du coup tout le reste
    if(getcwd(pwd,MAX_PATH_SIZE) == NULL){  // Getting the path
        printf("Error in 'path_shell' : couldn't execute 'getcwd'...\n");
        exit(1);
    }
    if ((pwd = realloc(pwd, sizeof(char) * (strlen(pwd) + 1))) == NULL) {   // Reallocating the path to the right size
        printf("Error in 'path_shell' : couldn't realloc...\n");
        exit(1);
    }
    return pwd;
}

int execute_commande_externe(char **commande_args, int has_pipe){
    return add_job(commande_args, (has_pipe > 0));
}

int change_precedent(char **prec,char *new){
    int size = strlen(new);
    free(*prec);
    *prec = malloc(sizeof(char)*(size+1));
    strcpy(*prec,new);
    
    return 0;
}

int execute_cd(char **commande_args,char **precedent){
    int size = 0;
    char *pwd = execute_pwd();

    while(commande_args[size] != NULL){
        size++;
    }
    if(size == 1 || strcmp( commande_args[1],"$HOME")==0 ){
        char *home = getenv("HOME");
        if(home == NULL){
            fprintf(stderr,"Erreur: la variable d'environnement HOME n'est pas définie\n");
            free(pwd);
            return 1;
        }
        chdir(home);
        change_precedent(precedent,pwd);
        free(pwd);
        return 0;
    }
    if(size > 2){
        free(pwd);
        fprintf(stderr,"Erreur: cd prend un seul %d argument\n", size);
        return 1;
    }
    if(strcmp(commande_args[1],"-") == 0){
        chdir(*precedent);
    }
    else{
        int n = chdir(commande_args[1]);
          if(n == -1){
            fprintf(stderr,"Erreur: le dossier n'existe pas\n");
            free(pwd);
            return 1;
        }
    }
    change_precedent(precedent,pwd);
    free(pwd);
    return 0;
}

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

int handle_kill(char **commande_args, int has_pipe) {
    bool wrong_args = false;
    if(len(commande_args) == 2){    // if we have command like "kill %1"
        if(start_with_char_then_digits(commande_args[1],'%')){ // if the second argument is an id string (starts with % and contains only digits)
            return kill_id(atoi(commande_args[1]+1));   // We remove the first character of the string (the %) and we convert it to an int
        }
        else{
            wrong_args = true;
        }
    }
    else if(len(commande_args) == 3){   // if we have command like "kill -9 %1"
        if(start_with_char_then_digits(commande_args[1],'-') && start_with_char_then_digits(commande_args[2],'%')){
            return send_signal_to_id(atoi(commande_args[2]+1),atoi(commande_args[1]+1));   // We remove the first characters of the strings (the - and %) and we convert them to an int
        }
        else{
            wrong_args = true;
        }

    }
    else{
        wrong_args = true;
    }

    if(wrong_args){
        return execute_commande_externe(commande_args, has_pipe);
    }
    return 0;
}


int handle_exit(char **commande_args, int fd, int last_return_code) {
    int numberOfJobs = getNbJobs();
    if(numberOfJobs != 0){  // If there are jobs running, we print an error message
        printf("Il y a %d jobs en cours d'execution\n",numberOfJobs);
        return 1;
    }
    else{
        if(len(commande_args) > 2){ // If there are more than 2 arguments, we print an error message
            printf("Erreur: exit prend un seul argument\n");
            return 1;
        }
        else if(len(commande_args) == 2){
            if(!is_number(commande_args[1])){   // If there is one argument and it's not a number, we print an error message
                printf("Erreur: argument de exit non valide\n");
                return 1;
            }
            else{   // If there is one argument and it's a number, we exit with the return code
                is_running = 0;
                return atoi(commande_args[1]);
            }
        }
        else{
            is_running = 0;
            if (fd != -1){
                char buf[1024];
                int n = read(fd, buf, 1024);
                if (n > 0) buf[n] = '\0';
                return atoi(buf);
            } else {
                return last_return_code;
            }
        }
    }
}


int handle_internal_commands(char **commande_args, int *last_return_code, char **precedent, int has_pipe) {
    int ret;
    int *fd = NULL;

    if(isRedirection(commande_args) != -1 && !has_pipe){
        fd = getDescriptorOfRedirection(commande_args);

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
    }

    if (strcmp(commande_args[0], "exit") == 0) {
        if (fd != NULL) {
            ret = handle_exit(commande_args, fd[0], *last_return_code);
        } 
        else {
            ret = handle_exit(commande_args, -1, *last_return_code);
        }
    } 
    else if (strcmp(commande_args[0], "?") == 0) {
        printf("%d\n", *last_return_code);
        ret = 0;
    } 
    else if (strcmp(commande_args[0], "cd") == 0) {
        ret = execute_cd(commande_args, precedent);
    } 
    else if (strcmp(commande_args[0], "jobs") == 0) {
        ret = print_all_jobs();
    } 
    else if (strcmp(commande_args[0], "kill") == 0) {
        ret = handle_kill(commande_args, has_pipe);
    } 
    else {
        return -1;  // -> donc on execute la commande externe
    }
    
    free(fd);
    return ret;
}


int main(int argc, char const *argv[]){
    
    char *input;
    char *precedent = execute_pwd();
    char **commande_args;

    int last_return_code = 0;
    int has_pipe;
    
    using_history();
    rl_outstream = stderr;

    while(is_running){

        char *path = path_shell("$ ", green, blue);
        
        input = readline(path);
        verify_done_jobs(); // We verify if there are jobs that are done, and we remove them from the list of jobs
        add_history(input);
        if(input == NULL){
            is_running = 0;
            continue;
        }
        free(path);

        remove_last_spaces(&input); // Removing the last spaces only after verifying that the input is not empty
        if(strlen(input) == 0){    // If the input is empty after removing the last spaces, we continue
            continue;
        }
        
        commande_args = get_tab_of_commande(input);
        if(commande_args == NULL){
            continue;
        }
        
<<<<<<< HEAD
        nb_pipes = nbPipes(commande_args);
        int result = handle_internal_commands(commande_args, &last_return_code, &precedent, nb_pipes);
        if (result != -1) {
            last_return_code = result;
        } else {
            last_return_code = execute_commande_externe(commande_args, nb_pipes);
=======
        has_pipe = nbPipes(commande_args);
        int result = handle_internal_commands(commande_args, &last_return_code, &precedent, has_pipe);
        if (result != -1) {
            last_return_code = result;
        } else {
            last_return_code = execute_commande_externe(commande_args, has_pipe);
>>>>>>> 5289b421a7fba4b8e0bc62ec728cc91e346dd4ac
        }

        free(commande_args);
        free(input);

    }

    // free(commande_args);
    // free(input);
    free(precedent);
    clear_history();
    
    return last_return_code;
}
