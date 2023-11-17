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
    int recup;

    if(pid == 0){
        int n = execvp(commande_args[0],commande_args);
        return n;
    }
    else{
        waitpid(pid, &recup, 0);
    }
    return recup;
}

int main(int argc, char const *argv[]){
    
    char *input;
    using_history();

    while(1){
        input = readline("MonPrompt> ");
        add_history(input);
        char **commande_args = get_tab_of_commande(input);

        int i = 0;
        while(commande_args[i] != NULL){
            printf("%s\n",commande_args[i]);
            i++;
        }
        if(strcmp(commande_args[0],"exit") == 0){
            break;
        }else if (strcmp(commande_args[0],"?") == 0){
            HIST_ENTRY *last_entry;
            int len = history_length;

            if (len > 0) {
                last_entry = history_get(len-1); 
                if (last_entry != NULL){
                    char * last_commande = malloc(sizeof(char)*strlen(last_entry->line));
                    strcpy(last_commande,last_entry->line);
                    printf ("%d", execute_commande_externe(get_tab_of_commande(last_commande)));
                    free(last_commande);
                }else{
                    fprintf(stderr, "Pas de commande précédente\n");
                } 
            } else {
                fprintf(stderr, "Historique vide\n");
            }
        }else{
            execute_commande_externe(commande_args);
        }
        

        free(commande_args);
    }
    
    return 0;
}
