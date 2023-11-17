#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#define MAX_PATH_SIZE 150

enum color {red,green,blue,yellow,cyan,white};

void color_switch(char **string,enum color c){

    char *new_string = calloc((strlen(*string)+1),sizeof(char));

    char *color = malloc(sizeof(char)*9);
    switch(c){
        case red:
            color = "\033[31m";
            break;
        case green:
            color = "\033[32m";
            break;
        case blue:
            color = "\033[34m";
            break;
        case yellow:
            color = "\033[33m";
            break;
        case cyan:
            color = "\033[36m";
            break;
        default:    // white
            color = "\033[37m";
            break;
    }

    strcat(new_string,color);
    strcat(new_string,*string);
    strcat(new_string,"\033[37m");

    free(*string);
    *string = new_string;
}

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

char *execute_pwd(){
    char *pwd = malloc(sizeof(char)*MAX_PATH_SIZE);
    if(getcwd(pwd,MAX_PATH_SIZE) == NULL){
        printf("Error in 'path_shell' : couldn't execute 'getcwd'...\n");
        exit(1);
    }
    if ((pwd = realloc(pwd, sizeof(char) * (strlen(pwd) + 1))) == NULL) {
        printf("Error in 'path_shell' : couldn't realloc...\n");
        exit(1);
    }
    return pwd;
}

char *path_shell(char *signe){
    char *pwd = execute_pwd();
    strcat(pwd,signe);
    return pwd;
}

int main(int argc, char const *argv[]){
    
    char *input;
    using_history();

    while(1){
        char *path = path_shell("$ ");
        color_switch(&path,green);
        input = readline(path);
        free(path);

        add_history(input);
        char **commande_args = get_tab_of_commande(input);

        int i = 0;
        while(commande_args[i] != NULL){
            printf("%s\n",commande_args[i]);
            i++;
        }
        if(strcmp(commande_args[0],"cd") == 0){
            chdir(commande_args[1]);
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
