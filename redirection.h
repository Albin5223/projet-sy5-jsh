#ifndef REDIRECTION_H
#define REDIRECTION_H

#define SIMPLE ">"
#define FORCE ">|"
#define SANS_ECRASEMENT ">>"
#define SORTIE_ERREUR "2>"
#define SORTIE_ERREUR_FORCE "2>|"
#define SORTIE_ERREUR_SANS_ECRASEMENT "2>>"


int numberOfRedirection(char **commande);
int isRedirection(char **commande);
char ** getCommandeOfRedirection (char **commande);
int* getDescriptorOfRedirection(char **commande); 

#endif