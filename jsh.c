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
#include "jobs.h"

/**
 * Represents the maximum size of the path
*/
#define MAX_PATH_SIZE 2048
/**
 * Represents the maximum size of shell_path
*/
#define TRONCATURE_SHELL 30

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

char** get_tab_of_commande  (char* commande){
    char *copy1 = strdup(commande);
    if (!copy1) exit(EXIT_FAILURE);

    int size = 0;
    for (char *x = strtok(copy1, " "); x != NULL; x = strtok(NULL, " ")) {
        size++;
    }
    free(copy1);

    char **commande_args= malloc(sizeof(char*) * (size + 1));
    if (commande_args == NULL) exit(EXIT_FAILURE);

    int i = 0;
    for (char *x = strtok(commande, " "); x != NULL; x = strtok(NULL, " ")) {
        commande_args[i] = x;
        if (!commande_args[i]) exit(EXIT_FAILURE);
        i++;
    }

    commande_args[i] = NULL;

    return commande_args;
}

int execute_commande_externe(char **commande_args){
    pid_t pid = fork();

    if(pid == 0){
        execvp(commande_args[0], commande_args);
        fprintf(stderr,"erreur avec commande : %s\n",commande_args[0]);
        exit(EXIT_FAILURE);
    }
    else{
        int s;
        waitpid(pid,&s,0);
        if(WIFEXITED(s)){
            return WEXITSTATUS(s);
        }
        return 0;
    }
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


int main(int argc, char const *argv[]){
    
    char *input;
    char *precedent = execute_pwd();

    int descripteur_sortie_standart;
    int descripteur_sortie_erreur;

    int copie_sortie_standart = dup(STDOUT_FILENO);
    int copie_sortie_erreur = dup(STDERR_FILENO);

    bool isBG = false;
   
    int last_return_code = 0;
    
    using_history();
    rl_outstream = stderr;

    while(1){
        isBG = false;
        descripteur_sortie_erreur = -1;
        descripteur_sortie_standart = -1;
        char *path = path_shell("$ ",magenta,blue);
        
        input = readline(path);
        if(input == NULL){
            exit(last_return_code);
        }
        free(path);

        remove_last_spaces(&input); // Removing the last spaces only after verifying that the input is not empty
        if(strlen(input) == 0){    // If the input is empty after removing the last spaces, we continue
            continue;
        }

        if(input[strlen(input)-1] == '&'){
            isBG = true;
            remove_last_char(&input);
            remove_last_spaces(&input);
        }

        if(strlen(input) == 0){    // If the input is empty after removing the &, we continue
            fprintf(stderr,"erreur avec commande : &\n");
            continue;
        }

        char **commande_args;
        add_history(input);
        
        commande_args = get_tab_of_commande(input);
        /*
        * Mise en place des descripteurs en cas de redirections
        * On sait que la dernière redirection a la priorité, donc on a juste besoin de 2 descripteur, le premier pour la redirection pour la sortie standart
        * et le deuxième pour la redirection dans la sortie erreur
        * 
        * Dans cette condition, si il y a la présence d'une ou plusieurs redirection, on va affecter les descripteurs correspondant puis excécuter la commande
        * avec la suite du code
        */
        if(isRedirection(commande_args)!=-1){
            int *fd = getDescriptorOfRedirection(commande_args);
            if(fd[0] != -1){
                descripteur_sortie_standart = fd[0];
            }
            if(fd[1] != -1){
                descripteur_sortie_erreur = fd[1];
            }
            //On récupère la commande cad : comm > fic, 
            commande_args = getCommandeOfRedirection(commande_args);
            free(fd);
            
        }
        
        if(strcmp(commande_args[0],"exit") == 0){
            if(commande_args[1] != NULL){
                exit(atoi(commande_args[1]));
            }
            else{
                exit(last_return_code);
            }
        }
        else if (strcmp(commande_args[0],"?") == 0){
            printf("%d\n",last_return_code);
            last_return_code = 0;
        }
        else if(strcmp(commande_args[0],"cd") == 0){
            last_return_code = execute_cd(commande_args, &precedent);
        }
        else{
            last_return_code = execute_commande_externe(commande_args);
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

        
        
        free(commande_args);
        free(input);
    }
    free(precedent);
    clear_history();
    
    
    return 0;
}
