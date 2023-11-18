#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_PATH_SIZE 150


char *execute_pwd(int max_size){
    char *pwd = malloc(sizeof(char)*max_size);
    if(getcwd(pwd,max_size) == NULL){
        printf("Error in 'path_shell' : couldn't execute 'getcwd'...\n");
        exit(1);
    }
    return pwd;
}

char** get_tab_of_commande(char *commande){
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
    int stat;

    if(pid == 0){
        int exec = execvp(commande_args[0],commande_args);
        return exec;
    }
    else{
        waitpid(pid, &stat, 0);
        return stat;
    }
}

void pointInterrogation(HIST_ENTRY *last_entry, int nb){
    if (strcmp(last_entry->line,"?") == 0){
        if (history_length < nb){
            fprintf(stderr, "Erreur : pas assez de commandes dans l'historique\n");
        }else{
            pointInterrogation(history_get(history_length - nb), nb + 1);
        }
    }else if (last_entry != NULL){
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
        fprintf(stderr, "Erreur : pas assez de commandes dans l'historique\n");
    }
}

int change_precedent(char **prec,char *new){
    int size = strlen(new);
    free(*prec);
    *prec = malloc(sizeof(char)*(size+1));
    strcpy(*prec,new);
    printf("------prec : %s\n",*prec);
    
    return 0;
}

int execute_cd(char **commande_args,char **precedent){
    int size = 0;
    char *pwd = execute_pwd(MAX_PATH_SIZE);

    while(commande_args[size] != NULL){
        size++;
    }
    if(size == 1 ||strcmp( commande_args[1],"$HOME")==0 ){
        chdir("/home");
        change_precedent(precedent,pwd);
        free(pwd);
        return 0;
    }
    if(size > 2){
        free(pwd);
        printf("Erreur: cd prend un seul argument\n");
        return -1;
    }
    if(strcmp(commande_args[1],"-") == 0){
        chdir(*precedent);
    }
    else{
        int n = chdir(commande_args[1]);
          if(n == -1){
            printf("Erreur: le dossier n'existe pas\n");
            free(pwd);
            return -1;
        }
    }
    change_precedent(precedent,pwd);
    free(pwd);
    return 0;
}

char *path_shell(int max_size, char signe){
    char *pwd = execute_pwd(max_size);

    int real_size = strlen(pwd)+1;
    if(real_size + 2 < max_size){
        pwd[real_size-1] = signe;
        pwd[real_size] = ' ';
        pwd[real_size+1] = '\0';
        if(realloc(pwd,sizeof(char)*(real_size+2)) == NULL){
            printf("Error in 'path_shell' : couldn't realloc...\n");
            exit(1);
        }
        return pwd;
    }
    else{
        printf("Error in 'path_shell' : name too long...\n");
        exit(1);
    }
}

int main(int argc, char const *argv[]){
    
    char *input;
    char *precedent = execute_pwd(MAX_PATH_SIZE);
   
    
    using_history();

    while(1){
        
        char *path = path_shell(MAX_PATH_SIZE,'$');
        
        input = readline(path);
        free(path);

        add_history(input);
        char **commande_args = get_tab_of_commande(input);

        if(strcmp(commande_args[0],"exit") == 0){
            break;
        }
        else if (strcmp(commande_args[0],"?") == 0){
            pointInterrogation(history_get(history_length - 1), 2);
        }
        else if(strcmp(commande_args[0],"cd") == 0){
            execute_cd(commande_args,&precedent);
        }
        else{
            execute_commande_externe(commande_args);
        }

        free(commande_args);
    }
    free(precedent);
    
    
    return 0;
}
