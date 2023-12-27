#include "substitution_process.h"
#include "utils.h"
#include "redirection.h"
#include "internalCommand.h"

/**
 * Retourne le nombre de subs '<(' dans le tableau de commandes
 * @param command_args tableau de commandes
 * @return le nombre de subs '<(' dans le tableau de commandes
*/
int nb_subs(char **command_args){
    int nb = 0;
    for (int i = 0; command_args[i] != NULL; i++){
        if (ok_subs(command_args, i)){
            nb++;
        }
    }
    return nb;
}

/**
 * Retourne true si lorsqu'on parcourt le tableau de commandes, et qu'on trouve '<(' on a bien ')' après
 * @param command_args
 * @param i
 * @return true si lorsqu'on parcourt le tableau de commandes, et qu'on trouve '<(' on a bien ')' après
*/
bool ok_subs(char **command_args, int i){
    if (strcmp(command_args[i], "<(") == 0){
        while(1){
            char *tmp = command_args[i++];
            if (tmp == NULL) break;
            if (strcmp(tmp, ")") == 0) return true;
        }
    }
    return false;
}

/**
 * Retourne un tableau de la première occurence de ce qui se trouve à l'intérieur de : '<(' et ')'
 * @param command_args
 * @param nb_subs
 * @return un tableau de la première occurence de ce qui se trouve à l'intérieur de : '<(' et ')'
*/
char **get_substitution_process(char **command_args, int nb_subs) {
    int size = 0;
    int start;
    for (int i = 0; i < nb_subs && command_args[i] != NULL; i++) {
        if (ok_subs(command_args, i)) {
            i++; // Passer le '<('
            start = i;
            while (strcmp(command_args[i], ")") != 0 && command_args[i] != NULL) {
                size++;
                i++;
            }
            break;
        }
    }

    char **tab_subs = malloc(sizeof(char*) * (size + 1));
    if (malloc_ok(tab_subs)) exit(1);

    int j = 0;
    for(int i = start; i < start + size; i++){
        tab_subs[j++] = command_args[i];
    }

    tab_subs[j] = NULL;  

    return tab_subs;
}

/**
 * cat -n <( echo "Header" ) <( ps a ) <( echo "Footer" ) 
 * cat <( echo "Header" | tr 'a-z' 'A-Z' ) <( ps a ) <( echo "Footer" | tr 'a-z' 'A-Z' )
 * diff <( echo "aaaz" ) <( echo "zzz" ) &
*/

/**
 * Retourne le tableau de commande avant le premier '<('
 * @param command_args
 * @return le tableau de commande avant le premier '<('
*/
char **get_main_command(char **command_args){
    int size = 0;
    while(command_args[size] != NULL && strcmp(command_args[size], "<(") != 0){
        size++;
    }

    char **tab = malloc(sizeof(char*) * (size + 1));
    if (malloc_ok(tab)) exit(1);

    for(int i = 0; i < size; i++){
        tab[i] = command_args[i];
    }

    tab[size] = NULL;
    return tab;
}

int execute_substitution_process(char **command_args, int nb_subs){
    int ret = 0;

    char tmp_files_name[nb_subs][256];
    char **main_command = get_main_command(command_args);

    char ***tab = malloc(sizeof(char**) * (nb_subs + 1));
    if (malloc_ok(tab) == 1) exit(1);
    tab[nb_subs] = NULL;
    command_args += len(main_command);
    for (int i = 0; i < nb_subs; i++){
        tab[i] = get_substitution_process(command_args, nb_subs);
        command_args += len(tab[i]) + 1;
    }

    for (int i = 0; i < nb_subs; i++) {
        snprintf(tmp_files_name[i], sizeof(tmp_files_name[i]), "/tmp/%d.tmp", i);
        int fd_tmp = open(tmp_files_name[i], O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd_tmp == -1) exit(1);

        if (fork() == 0) {
            dup2(fd_tmp, STDOUT_FILENO);
            close(fd_tmp);

            if (isInternalCommand(tab[i]) == 1){
                ret = execute_internal_command(tab[i]);
                exit(ret);
            }

            setpgid(0, 0); // Set the process group ID to the process ID
        
            int descripteur_sortie_standart = -1;
            int descripteur_sortie_erreur = -1;
            int descripteur_entree = -1; 

            if(isRedirectionEntree(tab[i]) != -1){
                descripteur_entree = getFichierEntree(tab[i]);
                if(descripteur_entree == -1){
                    dprintf(STDERR_FILENO,"bash: %d: %s.\n", getFichierEntree(tab[i]), strerror(errno));
                    exit(1);
                }
                dup2(descripteur_entree, STDIN_FILENO);
                tab[i] = getCommandeWithoutRedirectionEntree(tab[i]);
            }

            if(isRedirection(tab[i]) != -1){
                int *fd = getDescriptorOfRedirection(tab[i]);
                if(fd[0] != -1){
                    descripteur_sortie_standart = fd[0];
                }
                if(fd[1] != -1){
                    descripteur_sortie_erreur = fd[1];
                }

                if(isRedirectionStandart(tab[i]) != -1 && fd[0] == -1){
                    free(fd);
                    dprintf(STDERR_FILENO,"bash: sortie: %s\n", "cannot overwrite existing file");
                    exit(1);
                }
                if(isRedirectionErreur(tab[i]) != -1 && fd[1] == -1){
                    free(fd);
                    exit(1);
                }

                //On récupère la commande cad : comm > fic, 
                tab[i] = getCommandeOfRedirection(tab[i]);
                free(fd);
            }

            execvp(tab[i][0], tab[i]);
            exit(1);
        } else {
            close(fd_tmp);
            int status;
            wait(&status);
            if (WIFEXITED(status)){
                if (WEXITSTATUS(status) != 0){
                    return 1;
                }
            }
        }
    }

    for (int i = 0; i < nb_subs; i++) {
        wait(NULL);
    }

    int start = len(main_command);
    main_command = realloc(main_command, sizeof(char*) * (start + nb_subs + 1));
    if (malloc_ok(main_command) == 1) goto error;
    main_command[start + nb_subs] = NULL;

    for (int i = 0; i < nb_subs; i++) {
        main_command[start + i] = tmp_files_name[i];
    }

    if (fork() == 0) {
        if (isInternalCommand(main_command) == 1){
            ret = execute_internal_command(main_command);
        } else {
            execvp(main_command[0], main_command);
            goto error;
        }
    }

    wait(NULL);

    for (int i = 0; i < nb_subs; i++) {
        unlink(tmp_files_name[i]);
    }

    return ret;


error : 
    for (int i = 0; i < nb_subs; i++) {
        unlink(tmp_files_name[i]);
    }
    return 1;
}