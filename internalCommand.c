#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


#include <readline/readline.h>
#include <readline/history.h>

#include "redirection.h"
#include "internalCommand.h"
#include "jobs.h"
#include "kill.h"
#include "jobs-command.h"
#include "utils.h"


char *precedent = NULL;
int last_return_code;

void setLastRetrunCode(int n){
    last_return_code = n;
}

/**
 * @brief Check if a command is an internal command
 * @param commands The command to check
 * @return 1 if the command is an internal command, 0 if not
*/
int isInternalCommand(char ** commands){
    if (strcmp(commands[0], EXIT) == 0) return 1;
    if (strcmp(commands[0], CD) == 0) return 1;
    if (strcmp(commands[0], LAST) == 0) return 1;
    if (strcmp(commands[0], JOBS) == 0) return 1;
    if (strcmp(commands[0], KILL) == 0) return 1;
    if (strcmp(commands[0], FG) == 0) return 1;
    if (strcmp(commands[0], BG) == 0) return 1;
    return 0;
}

/**
 * @brief Change the previous directory
 * @return 0
*/
int change_precedent(char *new){
    int size = strlen(new);
    if(precedent != NULL){
        free(precedent);
    }
    precedent = malloc(sizeof(char)*(size+1));
    strcpy(precedent,new);

    return 0;
}

void liberePrecedent(){
    free(precedent);
}

/**
 * @brief Execute the cd command
 * @param commande_args The arguments of the command
*/
int execute_cd(char **commande_args){
    int size = 0;
    char *pwd = execute_pwd();

    while(commande_args[size] != NULL){
        size++;
    }
    if(size == 1 || strcmp( commande_args[1],"$HOME")==0 ){
        char *home = getenv("HOME");
        if(home == NULL){
            dprintf(STDERR_FILENO,"Erreur: la variable d'environnement HOME n'est pas définie\n");
            free(pwd);
            return 1;
        }
        chdir(home);
        change_precedent(pwd);
        free(pwd);
        return 0;
    }
    if(size > 2){
        free(pwd);
        dprintf(STDERR_FILENO,"Erreur: cd prend un seul %d argument\n", size);
        return 1;
    }
    if(strcmp(commande_args[1],"-") == 0){
        chdir(precedent);
    }
    else{
        int n = chdir(commande_args[1]);
          if(n == -1){
            dprintf(STDERR_FILENO,"Erreur: le dossier n'existe pas\n");
            free(pwd);
            return 1;
        }
    }
    change_precedent(pwd);
    free(pwd);
    return 0;
}

/**
 * @brief Execute the exit command
 * @param commande_args The arguments of the command
*/
int executeExit(char **commande_args){
    int numberOfJobs = getNbJobs();
    if(numberOfJobs != 0){  // If there are jobs running, we print an error message
        dprintf(STDERR_FILENO, "Il y a %d jobs en cours d'execution\n",numberOfJobs);
        return 1;
    }
    else{
        if(len(commande_args) > 2){ // If there are more than 2 arguments, we print an error message
            printf("Erreur: exit prend un seul argument\n");
            return -1;
        }
        else if(len(commande_args) == 2 && !is_number(commande_args[1])){   // If there is one argument and it's not a number, we print an error message
            printf("Erreur: argument de exit non valide\n");
            return -1;
        }
        else if(len(commande_args) == 2){   // If there is one argument and it's a number, we exit with the return code
            last_return_code = atoi(commande_args[1]);
            free(commande_args);
            free(precedent);
            clear_history();
            exit(last_return_code);
        }
        else{   // If there is no argument, we exit with the return code
            free(commande_args);
            free(precedent);
            clear_history();
            exit(last_return_code);
        }
    }
}

/**
 * @brief Execute the ? command
 * @return 0
*/
int executeLastVal(){
    printf("%d\n",last_return_code);
    return 0;
}

/**
 * @brief Execute the jobs command
 * @param commande_args The arguments of the command
*/
int executeJobs(char **commandeArgs){
    return executeinternalJobs(commandeArgs);
}

/**
 * @brief Execute the kill command
 * @param commande_args The arguments of the command
*/
int executeKill(char **commande_args){
    if(len(commande_args) == 2){    // if we have command like "kill %1"
        if(start_with_char_then_digits(commande_args[1],'%')){ // if the second argument is an id string (starts with % and contains only digits)
            return kill_id(atoi(commande_args[1]+1));   // We remove the first character of the string (the %) and we convert it to an int
        }
    }
    else if(len(commande_args) == 3){   // if we have command like "kill -9 %1"
        if(start_with_char_then_digits(commande_args[1],'-') && start_with_char_then_digits(commande_args[2],'%')){
            return send_signal_to_id(atoi(commande_args[2]+1),atoi(commande_args[1]+1));   // We remove the first characters of the strings (the - and %) and we convert them to an int
        }
        else if(start_with_char_then_digits(commande_args[1],'-') && is_number_strict(commande_args[2])){ // if we have command like "kill -9 pid"
            return kill(atoi(commande_args[2]),atoi(commande_args[1]+1));
        }
    }
    else if(len(commande_args) == 2 && is_number_strict(commande_args[1])){  // if we have command like "kill pid"
        return kill(atoi(commande_args[1]),SIGTERM);
    }

    char pourcentage = '%';
    dprintf(STDERR_FILENO,"Erreur: kill [-sig] [%cjob] \n",pourcentage);
    return -1;
}



int excecuteFG(char **command_args){
    if(len(command_args) == 2){
        if(start_with_char_then_digits(command_args[1],'%')){
            return fg_id(atoi(command_args[1]+1));
        }
    }

    return -1;
}

/**
 * @brief Get the last return code
 * @return The last return code
*/
int getLastRetrunCode(){
    return last_return_code;
}

/**
 * @brief Execute an internal command
 * @param commande_args The arguments of the command
 * @return 0 if the command is executed, -1 if not
*/
int executeInternalCommand(char **commande_args){
    int fd = -1;
    char buf[2048];
    
    if(isRedirectionEntree(commande_args) != -1){
        fd = getFichierEntree(commande_args);

        if(fd == -1){
            dprintf(STDERR_FILENO,"Erreur: impossible de lire l'entree\n");
            return -1;
        }
        
        if(read(fd,buf,2048) == -1){
            dprintf(STDERR_FILENO,"Erreur: impossible de lire l'entree\n");
            return -1;
        }
        buf[strlen(buf)-1] = '\0';
        int size = isRedirectionEntree(commande_args);
        commande_args[size] = NULL;
        if (is_number(buf)){
            last_return_code = 0;
        }

        close(fd);
    }

    if(isRedirection(commande_args) != -1){
        int *fd = getDescriptorOfRedirection(commande_args);

        if(isRedirectionStandart(commande_args) != -1 && fd[0] == -1){
            free(fd);
            dprintf(STDERR_FILENO,"bash: sortie: %s\n", "cannot overwrite existing file");
            return 1;
        }
        if(isRedirectionErreur(commande_args) != -1 && fd[1] == -1){
            free(fd);
            return 1;
        }

        //On récupère la commande cad : comm > fic, 
        int size = isRedirection(commande_args);
        commande_args[size] = NULL;
        close(fd[0]);
        close(fd[1]);
        free(fd);
        
    }

    if(strcmp(commande_args[0],"exit") == 0){
        return executeExit(commande_args);
    }
    if (strcmp(commande_args[0],"?") == 0){
        return executeLastVal();
    }
    if(strcmp(commande_args[0],"jobs") == 0){
        return executeJobs(commande_args);
    }
    if(strcmp(commande_args[0],"kill") == 0){
        return executeKill(commande_args);
    }
    if(strcmp(commande_args[0],"cd") == 0){
        return execute_cd(commande_args);
    }
    if(strcmp(commande_args[0],"fg") == 0){
        return excecuteFG(commande_args);
    }

    dprintf(STDERR_FILENO,"Erreur: commande interne non reconnue\n");
    return -1;
}
