#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "pipe.h"
#include "utils.h" 

int nbPipes(char **commande){
    int i = 0;
    int nb = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,PIPE) == 0){
            nb++;        
        }
        i++;
    }

    return nb;
}

char ** noPipe(char ** commande, int nb) {
    int size = len(commande);
    char **tab_no_pipes = malloc(sizeof(char*) * ((size - nb) + 1));
    int j = 0;
    int tmpSize = 256; 
    char *tmp = malloc((char) tmpSize);
    tmp[0] = '\0'; 

    for (int i = 0; i < size; i++) {
        if (strcmp(commande[i], PIPE) != 0) {
            if (strlen(tmp) + strlen(commande[i]) + 2 > tmpSize) {
                tmpSize *= 2;
                tmp = realloc(tmp, tmpSize);
            }
            strcat(tmp, commande[i]);
            strcat(tmp, " ");
        } else {
            tab_no_pipes[j++] = strdup(tmp);
            tmp[0] = '\0';
        }
    }

    if (tmp[0] != '\0') {
        tab_no_pipes[j++] = strdup(tmp); 
    }

    tab_no_pipes[j] = NULL; 
    free(tmp);

    return tab_no_pipes;
}


int doPipe(char **commande_args, int nb){
    int pipes_fd[nb - 1][2];
    pid_t pid;
    int i;

    for (i = 0; i < nb - 1; i++) {
        if (pipe(pipes_fd[i]) != 0){
            perror("doPipe");
            exit(1);
        }
    }

    for (i = 0; i < nb; i++) {
        pid = fork();
        if (pid == -1) {
            perror("doPipe, fork");
            exit(1);
        }

        if (pid == 0) {
            if (i > 0) {
                if (dup2(pipes_fd[i - 1][0], STDIN_FILENO) < 0) {
                    perror("doPipe, dup2");
                    exit(1);
                }
            }

            if (i < nb - 1) {
                if (dup2(pipes_fd[i][1], STDOUT_FILENO) < 0) {
                    perror("doPipe, dup2");
                    exit(1);
                }
            }

            for (int j = 0; j < nb - 1; j++) {
                close(pipes_fd[j][0]);
                close(pipes_fd[j][1]);
            }

            char **commande_tab_i = get_tab_of_commande(commande_args[i]);
            execvp(commande_tab_i[0], commande_tab_i);

            fprintf(stderr,"erreur avec commande : %s\n",commande_tab_i[0]);
            exit(1);
        }
    }

    for (i = 0; i < nb - 1; i++) {
        close(pipes_fd[i][0]);
        close(pipes_fd[i][1]);
    }

    int status;
    int ret = 0;
    for (i = 0; i < nb; i++) {
        waitpid(-1, &status, 0);
        if (WIFEXITED(status)) {
            int child_ret = WEXITSTATUS(status);
            if (child_ret != 0) {
                ret = child_ret;
            }
        }
    }

    return ret;
}


char **getCommandOfPipe(char **commande_args){
    int size = 0;
    while(1){
        char *tmp = commande_args[size];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,PIPE) == 0){
            break;
        }
        size++;
    }
    char **tab = malloc(sizeof(char*) * (size + 1));

    for(int i = 0; i < size; i++){
        tab[i] = commande_args[i];
    }

    tab[size] = NULL;
    return tab;
}

int makePipe(char **commande_args){
    int size = nbPipes(commande_args)+1;
    char ***tab = malloc(sizeof(char**) * size);

    for(int i = 0; i < size; i++){
        tab[i] = getCommandOfPipe(commande_args);
        commande_args = commande_args + len(tab[i]) + 1; 
    }

    int fd[size-1][2];
    for(int i = 0;i<size-1;i++){
        if(pipe(fd[i]) == -1){
            perror("pipe");
            return 1;
        }
    }

    pid_t pid = fork();
    int pid2;
    switch (pid){
    case -1:
        perror("fork");
        break;
    case 0 :
        for(int i = 0;i<size-1;i++){
            pid2 = fork();
            switch (pid2){
                case -1:
                    perror("fork");
                    break;
                case 0:
                    if(i == 0){
                        dup2(fd[i][1], STDOUT_FILENO);
                        for(int j = 0;j<size-1;j++){
                            close(fd[j][0]);
                            close(fd[j][1]);
                        }
                        execvp(tab[i][0], tab[i]);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                    else{
                        dup2(fd[i-1][0], STDIN_FILENO);
                        dup2(fd[i][1], STDOUT_FILENO);
                        for(int j = 0;j<size-1;j++){
                            close(fd[j][0]);
                            close(fd[j][1]);
                        }
                        execvp(tab[i][0], tab[i]);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
            }
        }  
    default:
        dup2(fd[size-2][0], STDIN_FILENO);
        for(int i = 0;i<size-1;i++){
            close(fd[i][0]);
            close(fd[i][1]);
        }
        execvp(tab[size-1][0], tab[size-1]);
        perror("execvp");
        exit(EXIT_FAILURE);
        break;
    }

    // Libérer la mémoire allouée
    for (int i = 0; i < size; i++) {
        free(tab[i]);
    }
    free(tab);

    return 0;
}