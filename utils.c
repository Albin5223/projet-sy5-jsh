#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>


/**
 * Represents the maximum size of the path
*/
#define MAX_PATH_SIZE 2048

/**
 * @brief Return the number of digits of the number n (including the signe)
*/
int number_length(int n){
    int i = 0;
    if(n < 0){
        i++;
        n = -n;
    }
    do {
        n = n/10;
        i++;
    } while(n != 0);
    return i;
}

/*
* @brief Free the tab of string    
*/
void free_tab(char ** tab) {
    int i = 0;
    while (tab[i] != NULL) {
        free(tab[i]);
        i++;
    }
    free(tab);
}

/**
 * @brief Truncate the string to the size of truncate_size
 * @param original The string to truncate
*/
void truncate_string(char **original, int truncate_size) {
    int len = strlen(*original);
    if (len > truncate_size-3) {    // minus 3 because of the 3 dots
        char *new_string = malloc(truncate_size+1); // 3 dots + truncate_size-3 characters + null terminator
        snprintf(new_string, truncate_size+1, "...%s",(*original + len) - (truncate_size-3));
        free(*original);
        *original = new_string;
    }
}

/**
 * @brief Remove the last spaces of the string
 * @param str The string to remove the last spaces
*/
void remove_last_spaces(char **str) {
    int len = strlen(*str);
    if(len == 0) return;
    int i = len-1;
    while ((*str)[i] == ' ' && i >= 0) {
        (*str)[i] = '\0';
        i--;
    }
}

/**
 * @brief Remove the last character of the string
 * @param str The string to remove the last character
*/
void remove_last_char(char **str) {
    int len = strlen(*str);
    (*str)[len-1] = '\0';
}

/**
 * @brief return the length of the tab of string
 * @param str The tab of string
*/
int len (char ** str) {
    int i = 0;
    while (str[i] != NULL) {
        i++;
    }
    return i;
}

char** get_tab_of_commande (char* commande){
    char *copy1 = strdup(commande);
    if (!copy1) exit(1);

    int size = 0;
    for (char *x = strtok(copy1, " "); x != NULL; x = strtok(NULL, " ")) {
        size++;
    }
    free(copy1);

    char **commande_args= malloc(sizeof(char*) * (size + 1));
    if (commande_args == NULL) exit(1);

    int i = 0;
    for (char *x = strtok(commande, " "); x != NULL; x = strtok(NULL, " ")) {
        commande_args[i] = x;
        if (!commande_args[i]) exit(1);
        i++;
    }
    commande_args[i] = NULL;

    return commande_args;
}

/**
 * @brief return true if the string start with the char c then continue with digits
 * @param str The string
 * @param c The char
*/
bool start_with_char_then_digits(char *str, char c) {
    if(str == NULL) return false;
    int len = strlen(str);
    if(len < 2) return false;
    if(str[0] != c) return false;
    for(int i = 1; i < len; i++) {
        if(str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

bool is_number(const char *str) {
    if (*str == '-') {
        str++;
    }
    while(*str) {
        if(!isdigit(*str)) {
            return false;
        }
        str++;
    }
    return true;
}

bool is_number_strict(const char *str){
    while(*str) {
        if(!isdigit(*str)) {
            return false;
        }
        str++;
    }
    return true;
}


int getNumberOfString(char *s){
    if(*s == '-'){
        return atoi(s+1);
    }
    return atoi(s);
}

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

char **add_tab_of_commande(char **tab, char **commande) {
    int i = 0;
    while (tab[i] != NULL) {
        i++;
    }
    int j = 0;
    while (commande[j] != NULL) {
        tab[i] = commande[j];
        i++;
        j++;
    }
    tab[i] = NULL;
    return tab;
}
    
/**
 * @brief Convert an integer to a string
 * @param n The integer to convert
 * @return The string --> must be freed
*/
char *int_to_string(int n) {
    int len = number_length(n);
    char *str = malloc(sizeof(char) * (len + 1));
    if (!str) exit(1);
    snprintf(str, len + 1, "%d", n);
    *(str + len) = '\0';
    return str;
}


void print_tab(char **tab) {
    int i = 0;
    while(1){
        char *tmp = tab[i];
        if(tmp == NULL) {
            printf("NULL \n");
            break;
        }
        printf("%s ", tmp);
        i++;
    }
}

void affiche_tab(char **tab){
    int i = 0;
    while(tab[i] != NULL){
        printf("%s ",tab[i]);
        i++;
    }
    printf("\n");
}

char **get_command_of(char **commande_args, char *x){
    int size = 0;
    while(1){
        char *tmp = commande_args[size];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,x) == 0){
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
    

void ignore_all_signals() {
    struct sigaction sa = {0};

    sa.sa_handler = SIG_IGN;
    // Ignore all signals except SIGKILL and SIGSTOP and SIGCHLD
    for (int i = 1; i < NSIG; i++) {
        if(i == SIGINT || i == SIGTERM || i == SIGTTIN || i == SIGQUIT || i == SIGTTOU || i == SIGTSTP || i == SIGALRM){
            sigaction(i, &sa, NULL);
        }
    }
}

void dont_ignore_all_signals() {
    struct sigaction sa = {0};

    sa.sa_handler = SIG_DFL;
    
    // Ignore all signals
    for (int i = 1; i < NSIG; i++) {
        sigaction(i, &sa, NULL);
    }
}

volatile pid_t other_pid;

void signal_handler(int sig) {
    kill(other_pid, sig);
}

void redirect_signals_to(pid_t pid){
    other_pid = pid;
    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    // Redirect all signals except SIGKILL and SIGSTOP and SIGCHLD
    for (int i = 1; i < NSIG; i++) {
        if (i != SIGKILL && i != SIGSTOP && i != SIGCHLD) {
            sigaction(i, &sa, NULL);
        }
    } 
}



void prettyPrint(char **commands,int fd){
    int i = 0;
    while(1){
        char *tmp = commands[i];
        if(tmp == NULL) {
            dprintf(fd,"NULL \n");
            break;
        }
        dprintf(fd,"%s ", tmp);
        i++;
    }
    printf("\n");
}