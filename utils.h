#ifndef UTILS_H
#define UTILS_H



int number_length(int n);
void truncate_string(char **original, int truncate_size);
void remove_last_spaces(char **str);
void remove_last_char(char **str);
int len (char ** str);
void free_tab(char ** tab);
char** get_tab_of_commande (char* commande);
bool start_with_char_then_digits(char *str, char c);
int is_number(const char *str);
char *execute_pwd();
char **add_tab_of_commande(char **tab, char **commande);
int getNumberOfString(char *s);
bool is_number_strict(const char *str);
char *int_to_string(int n);
void print_tab(char **tab);
char **get_command_of(char **commande_args, char *x);
void affiche_tab(char **tab);
void ignore_all_signals();
void dont_ignore_all_signals();
void redirect_signals_to(pid_t pid);

void prettyPrint(char **commands,int fd);

#endif