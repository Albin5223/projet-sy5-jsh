#ifndef REDIRECTION_H
#define REDIRECTION_H

#define SIMPLE ">"
#define FORCE ">|"
#define SANS_ECRASEMENT ">>"
#define ENTREE "<"
#define SORTIE_ERREUR "2>"
#define SORTIE_ERREUR_FORCE "2>|"
#define SORTIE_ERREUR_SANS_ECRASEMENT "2>>"

int isRedirectionErreur(char **commande);
int isRedirectionStandart(char **commande);
int isRedirectionEntree(char **commande);
int getFichierEntree(char **commande);
int numberOfRedirection(char **commande);
int isRedirection(char **commande);
char ** getCommandeOfRedirection (char **commande);
int* getDescriptorOfRedirection(char **commande); 
int nbPipes(char **commande);
char ** noPipe(char ** commande, int nb);

#endif