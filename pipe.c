#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "pipe.h"
#include "utils.h" 

int numberOfPipes(char **commande){
    int i = 0;
    int nb = 0;
    int numberOfParenthese = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,"<(")==0){
            numberOfParenthese++;
        }
        if(strcmp(tmp,")")==0){
            numberOfParenthese--;
        }
        if(strcmp(tmp,PIPE) == 0 && numberOfParenthese == 0){
            nb++;
        }
        i++;
    }

    return nb;
}

bool isPipe(char **commande){
    int i = 0;
    int numberOfParenthese = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,"<(")==0){
            numberOfParenthese++;
        }
        if(strcmp(tmp,")")==0){
            numberOfParenthese--;
        }
        if(strcmp(tmp,PIPE) == 0 && numberOfParenthese == 0){
            return true;
        }
        i++;
    }

    return false;
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


