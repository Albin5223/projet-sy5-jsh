#ifndef UTILS_H
#define UTILS_H

int number_length(int n);
void truncate_string(char **original, int truncate_size);
void remove_last_spaces(char **str);
void remove_last_char(char **str);
int len (char ** str);
int len_triple(char ***str);
void free_tab(char ** tab);
char** get_tab_of_commande (char* commande);

#endif