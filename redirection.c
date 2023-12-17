#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

#include "redirection.h"
#include "utils.h"

/**
 * @brief Retourne le nombre de redirection dans la commande
*/
int numberOfRedirection(char **commande){
    int i = 0;
    int number = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,SIMPLE)==0){
            number++;        
        }
        if(strcmp(tmp,FORCE)==0){
            number++;        
        }
        if(strcmp(tmp,SANS_ECRASEMENT)==0){
            number++;        
        }
        if(strcmp(tmp,SORTIE_ERREUR)==0){
            number++;        
        }
        if(strcmp(tmp,SORTIE_ERREUR_FORCE)==0){
            number++;        
        }
        if(strcmp(tmp,SORTIE_ERREUR_SANS_ECRASEMENT)==0){
            number++;
        }
        if(strcmp(tmp,ENTREE)==0){
            number++;
        }
        
        i++;
    }

    return number;
}

int isRedirectionEntree(char **commande){
    int i = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            return -1;  // break;
        }
        if(strcmp(tmp,ENTREE)==0){
            return i;
        }
        i++;
    }
    return -1;
}

int getFichierEntree(char **commande){
    int index = isRedirectionEntree(commande);
    if(index == -1){
        return -1;
    }
    char * r = commande[index+1];

    int fd = open(r,O_RDONLY);
    return fd;
}

/**
 * @brief Retourne si la commande contient une redirection standard, si oui retourne l'index de la redirection
*/
int isRedirectionStandart(char **commande){
    int i = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,SIMPLE)==0){
            return i;
        }
        if(strcmp(tmp,FORCE)==0){
            return i;
        }
        if(strcmp(tmp,SANS_ECRASEMENT)==0){
            return i;
        }
        if(strcmp(tmp,ENTREE)==0){
            return i;
        }
        i++;
    }

    return -1;
}

/**
 * @brief Retourne si la commande est une redirection d'erreur, si oui retourne l'index de la redirection
*/
int isRedirectionErreur(char **commande){
    int i = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            break;
        }
        if(strcmp(tmp,SORTIE_ERREUR)==0){
            return i;
        }
        if(strcmp(tmp,SORTIE_ERREUR_FORCE)==0){
            return i;
        }
        if(strcmp(tmp,SORTIE_ERREUR_SANS_ECRASEMENT)==0){
            return i;
        }
        i++;
    }

    return -1;
}

/**
 * @brief Retourne si la commande contient une redirection, si oui retourne l'index de la redirection
*/
int isRedirection(char **commande){
    int standard = isRedirectionStandart(commande);
    int erreur = isRedirectionErreur(commande);

    if(standard > 0 && erreur > 0){
        return standard < erreur ? standard : erreur;
    }
    else if(standard > 0){
        return standard;
    }
    else if(erreur > 0){
        return erreur;
    }
    else{
        return -1;
    }

    // int i = 0;
    // while(1){
    //     char *tmp = commande[i];
    //     if (tmp == NULL){
    //         return -1;
    //     }
    //     if(strcmp(tmp,SIMPLE)==0){
    //         return i;
    //     }
    //     if(strcmp(tmp,FORCE)==0){
    //         return i;
    //     }
    //     if(strcmp(tmp,SANS_ECRASEMENT)==0){
    //         return i;
    //     }
    //     if(strcmp(tmp,SORTIE_ERREUR)==0){
    //         return i;
    //     }
    //     if(strcmp(tmp,SORTIE_ERREUR_FORCE)==0){
    //         return i;
    //     }
    //     if(strcmp(tmp,SORTIE_ERREUR_SANS_ECRASEMENT)==0){
    //         return i;
    //     }
    //     if(strcmp(tmp,ENTREE)==0){
    //         return i;
    //     }
    //     i++;
    // }

    // return -1;
}

/** ex : on passe de : 'cmd > fic' à 'cmd'
 * @brief Retourne la commande mais elle s'arrete avant la redirection
*/
char ** getCommandeOfRedirection (char **commande){
    int size = isRedirection(commande);
    char **newCommande= malloc(sizeof(char*) * (size + 1));
    check_malloc(newCommande);

    for(unsigned i = 0; i < size; i++){
        newCommande[i] = commande[i];
    }

    newCommande[size] = NULL;
    for (unsigned i = size; commande[i] != NULL; i++) {
        free(commande[i]);
    }

    free(commande);
    return newCommande;
}


int* getDescriptorOfRedirection(char **commande){
    int *fd = malloc(sizeof(int) * 3);
    fd[0] = -1;   // entree standard
    fd[1] = -1;   // sortie standard
    fd[2] = -1;   // sortie erreur

    int index_err = isRedirectionErreur(commande);
    int index_std = isRedirectionStandart(commande);

    char *standard = NULL;
    char *erreur = NULL;

    /*
    * On recupere le dernier fichier de redirection pour la sortie standard et la sortie erreur
    */
    // while(1){
    //     char *tmp = commande[i];
    //     if(tmp == NULL){
    //         break;
    //     }
    //     if(strcmp(tmp,SIMPLE) == 0 || strcmp(tmp,FORCE) == 0 || strcmp(tmp,SANS_ECRASEMENT) == 0 || strcmp(tmp,ENTREE) == 0){
    //         standard = commande[i+1];
    //         index_std = i;
    //     }
    //     if(strcmp(tmp,SORTIE_ERREUR) == 0 || strcmp(tmp,SORTIE_ERREUR_FORCE) == 0 || strcmp(tmp,SORTIE_ERREUR_SANS_ECRASEMENT) == 0){
    //         erreur = commande[i+1];
    //         index_err = i;
    //     }
    //     i++;
    // }

    /*
    *On ouvre le fichier en fonction de la redirection
    */

    if(index_err > 0){
        erreur = commande[index_err + 1];

        if(strcmp(commande[index_err],SORTIE_ERREUR)==0){
            fd[2] = open(erreur,O_WRONLY|O_CREAT|O_EXCL,0666);
        }
        else if(strcmp(commande[index_err],SORTIE_ERREUR_FORCE)==0){
            fd[2] = open(erreur,O_WRONLY|O_CREAT|O_TRUNC,0666);
        }
        else{   // SORTIE_ERREUR_SANS_ECRASEMENT
            fd[2] = open(erreur,O_WRONLY|O_CREAT|O_APPEND,0666);
        }
    }
    if(index_std > 0){
        standard = commande[index_std + 1];

        if(strcmp(commande[index_std],SIMPLE)==0){
            fd[1] = open(standard,O_WRONLY|O_CREAT|O_EXCL,0666);
        }
        else if(strcmp(commande[index_std],FORCE)==0){
            fd[1] = open(standard,O_WRONLY|O_CREAT|O_TRUNC,0666);
        }
        else if(strcmp(commande[index_std],SANS_ECRASEMENT)==0){
            fd[1] = open(standard,O_WRONLY|O_CREAT|O_APPEND,0666);
        }
        else{   // ENTREE
            fd[0] = open(standard,O_RDONLY,0666);
        }
    }


    /*
    * On redirige l'entree standard 0 vers fd[0] si il est superieur a 0
    * On redirige la sortie standard 1 vers fd[0] si il est superieur a 0
    * On redirige la sortie erreur 2 vers fd[1] si il est superieur a 0
    */

    if(fd[0] > 0){
        dup2(fd[0], 0);
    }
    if(fd[1] > 0){
        dup2(fd[1], 1);
    }
    if(fd[2] > 0){
        dup2(fd[2], 2);
    }
    return fd;
}
