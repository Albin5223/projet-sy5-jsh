#ifndef INTERNALCOMMAND_H
#define INTERNALCOMMAND_H
#define EXIT "exit"
#define CD "cd"
#define LAST "?"
#define JOBS "jobs"
#define KILL "kill"
#define BG "bg"
#define FG "fg"

int getLastRetrunCode();
int executeInternalCommand(char **commande_args);
int isInternalCommand(char ** commands);
int change_precedent(char *newPrecedent);
int execute_cd(char **commande_args);
void libere_precedent();
void setLastRetrunCode(int code);
int executeExit(char **commande_args);

#endif