#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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