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
        int exec = execvp(commande_args[0],commande_args);
        return exec;
    }
    else{
        waitpid(pid, &recup, 0);
        return recup;
    }
}

void pointInterrogation(HIST_ENTRY *last_entry){
    if (last_entry != NULL){
        char * last_commande = malloc(sizeof(char)*strlen(last_entry->line));
        if (last_commande == NULL){
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            exit(EXIT_FAILURE);
        }

        strcpy(last_commande,last_entry->line);
        int recup = execute_commande_externe(get_tab_of_commande(last_commande));
        if (recup == -1) fprintf(stderr, "Erreur d'exécution lors de la commande : %s\n",last_commande);
        free(last_commande);
    }else{
        fprintf(stderr, "Pas de commande précédente\n");
    } 
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
            pointInterrogation(history_get(history_length - 1));
        }else{
            execute_commande_externe(commande_args);
        }
        

        free(commande_args);
    }
    
    return 0;
}
