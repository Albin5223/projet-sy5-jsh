#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_PATH_SIZE 150

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

char *execute_pwd(int max_size){
    char *pwd = malloc(sizeof(char)*max_size);
    if(getcwd(pwd,max_size) == NULL){
        printf("Error in 'path_shell' : couldn't execute 'getcwd'...\n");
        exit(1);
    }
    return pwd;
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
    using_history();

    while(1){
        char *path = path_shell(MAX_PATH_SIZE,'$');
        input = readline(path);
        free(path);

        add_history(input);
        char **commande_args = get_tab_of_commande(input);

        int i = 0;
        while(commande_args[i] != NULL){
            printf("%s\n",commande_args[i]);
            i++;
        }
        if(strcmp(commande_args[0],"exit") == 0){
            break;
        }
        else{
            execute_commande_externe(commande_args);
        }
        

        free(commande_args);
    }
    
    return 0;
}
