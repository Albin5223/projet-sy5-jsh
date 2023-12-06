#ifndef PIPE_H
#define PIPE_H

#define PIPE "|"

int nbPipes(char **commande);
char ** noPipe(char ** commande, int nb);
int doPipe(char **commande_args, int nb);
int makePipe(char **commande_args);

#endif