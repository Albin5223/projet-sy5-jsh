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
    pid_t child_pids[nb_subs];

    char ***tab = malloc(sizeof(char**) * (nb_subs + 1));
    if (malloc_ok(tab) == 1) exit(EXIT_FAILURE);
    tab[nb_subs] = NULL;
    command_args += len(main_command);
    
    for (int i = 0; i < nb_subs; i++){
        tab[i] = get_substitution_process(command_args, nb_subs);
        command_args += len(tab[i]) + 1;

        snprintf(tmp_files_name[i], sizeof(tmp_files_name[i]), "/tmp/%d.tmp", i);
        int fd_tmp = open(tmp_files_name[i], O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd_tmp == -1) {
               ret = 1;
            goto cleanup;
        }

        child_pids[i] = fork();
        if (child_pids[i] == -1) {
               close(fd_tmp);
            ret = 1;
            goto cleanup;
        }

        if (child_pids[i] == 0) { // Processus fils
            dup2(fd_tmp, STDOUT_FILENO);
            close(fd_tmp);

            // Exécution de la commande interne ou externe
            if (isInternalCommand(tab[i]) == 1){
                ret = executeInternalCommand(tab[i]);
                exit(ret);
            }

            setpgid(0, 0);
            execvp(tab[i][0], tab[i]);
            exit(EXIT_FAILURE);
        } else {
            close(fd_tmp);
        }
    }

    for (int i = 0; i < nb_subs; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            ret = 1;
        }
    }

    int start = len(main_command);
    main_command = realloc(main_command, sizeof(char*) * (start + nb_subs + 1));
    if (malloc_ok(main_command) == 1) {
        ret = 1;
        goto cleanup;
    }
    main_command[start + nb_subs] = NULL;

    for (int i = 0; i < nb_subs; i++) {
        main_command[start + i] = tmp_files_name[i];
    }

    if (fork() == 0) {
        if (isInternalCommand(main_command) == 1){
            ret = executeInternalCommand(main_command);
            exit(ret);
        } else {
            execvp(main_command[0], main_command);
            exit(EXIT_FAILURE);
        }
    }

    wait(NULL);

cleanup:
    // Nettoyage des ressources
    for (int i = 0; i < nb_subs; i++) {
        unlink(tmp_files_name[i]);
        free(tab[i]);
    }
    free(tab);
    return ret;
}