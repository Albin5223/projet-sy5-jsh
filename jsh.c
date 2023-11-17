#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

char** get_tab_of_commande  (char *commande){
    int size = 0;
    for(int i = 0; i<strlen(commande);i++){
        if(commande[i] == ' '){
            size++;
        }
    }
    int i = 0;
    char **commande_args= malloc(sizeof(char*)*(size+1));
    char *token = strtok(commande," ");
    while(token != NULL){
        commande_args[i] = token;
        token = strtok(NULL," ");
        i++;
    }
    commande_args[i] = NULL;

    return commande_args;
}

int execute_commande_externe(char **commande_args){
    pid_t pid = fork();
    if(pid == 0){
        int n = execvp(commande_args[0],commande_args);
        return n;
    }
    else{
        wait(NULL);
    }
    return 0;
}


int execute_cd(char **commande_args,char *precedent){
    int size = 0;
    while(commande_args[size] != NULL){
        size++;
    }
    if(size == 1 ||strcmp( commande_args[1],"$HOME")==0 ){
        chdir("/home");
        return 0;
    }
    if(size > 2){
        printf("Erreur: cd prend un seul argument\n");
        return -1;
    }
    if(strcmp(commande_args[1],"-") == 0){
        chdir(precedent);
        return 0;
    }
    
    int n = chdir(commande_args[1]);
    return n;
}

int main(int argc, char const *argv[]){
    
    char *input;
    char *precedent;
    using_history();

    while(1){
        input = readline("MonPrompt> ");
        add_history(input);
        char **commande_args = get_tab_of_commande(input);

        if(strcmp(commande_args[0],"exit") == 0){
            break;
        }
        if(strcmp(commande_args[0],"cd") == 0){
            int n = execute_cd(commande_args,precedent);
            printf("----%d\n",n);
        }
        else{
            execute_commande_externe(commande_args);
        }
        

        free(commande_args);
    }
    
    
    return 0;
}
