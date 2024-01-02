#include "substitution_process.h"
#include "utils.h"
#include "redirection.h"
#include "internalCommand.h"
#include "jobs.h"
#include "pipe.h"

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

int is_valid_fic(char *file){
    if (file == NULL) return 0;
    int fd = open(file, O_RDONLY);
    if (fd == -1) return -1;
    close(fd);
    return 0;
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
    if (tab_subs == NULL) exit(1);

    int j = 0;
    for(int i = start; i < start + size; i++){
        tab_subs[j++] = command_args[i];
    }

    tab_subs[j] = NULL;  

    return tab_subs;
}

/**
 * cat -n <( echo "Header" ) <( ps a ) <( echo "Footer" ) 
 * cat -n <( echo "Header" ) fichierTest <( echo "Footer" ) 
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
    if (tab == NULL) exit(1);

    for(int i = 0; i < size; i++){
        tab[i] = command_args[i];
    }

    tab[size] = NULL;
    return tab;
}

int execute_substitution_process(char **command_args, int nb_subs){
    if (isRedirection(command_args) != -1) return 1;

    int ret = 0;

    char tmp_files_name[nb_subs][256];
    char **main_command = get_main_command(command_args);

    char ***tab = malloc(sizeof(char**) * (nb_subs + 1));
    if (tab == NULL) exit(1);
    tab[nb_subs] = NULL;
    command_args += len(main_command);
    
    for (int i = 0; i < nb_subs; i++){
        if (is_valid_fic(command_args[0]) == 0){
            printf("Fichier %s déjà existant\n", command_args[0]);
            snprintf(tmp_files_name[i], sizeof(tmp_files_name[i]), "%s", command_args[0]);
            command_args += strlen(command_args[0]) + 1;
            continue;
        }

        char x_name[256];
        snprintf(x_name, sizeof(x_name), "/tmp/%d", i + 10);  
        // snprintf(x_name, sizeof(x_name), "/dev/fd/%d", i + 10);    // + 10, pour éviter les conflits avec les autres fichiers temporaires
        int fd = open(x_name, O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd == -1) {
            ret = 1;
            goto cleanup;
        }
        memcpy(tmp_files_name[i], x_name, strlen(x_name) + 1);
        
        tab[i] = get_substitution_process(command_args, nb_subs);
        command_args += len(tab[i]) + 1;
        int fd_tmp = open(tmp_files_name[i], O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd_tmp == -1) {
            ret = 1;
            goto cleanup;
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(fd_tmp);
            ret = 1;
            goto cleanup;
        }

        if (pid == 0) { // Processus fils
            dup2(fd_tmp, STDOUT_FILENO);
            close(fd_tmp);

            // Exécution de la commande interne ou externe
            if (isInternalCommand(tab[i]) == 1){
                ret = executeInternalCommand(tab[i]);
                exit(ret);
            }

            if (isPipe(tab[i]) == 1){
                ret = add_job_command_with_pipe(tab[i], false);
                exit(ret);
            }

            // setpgid(0, 0);
            execvp(tab[i][0], tab[i]);
            exit(EXIT_FAILURE);
        } else {
            close(fd_tmp);
        }
    }

    for (int i = 0; i < nb_subs; i++) {
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            ret = 1;
        }
    }

    int start = len(main_command);
    main_command = realloc(main_command, sizeof(char*) * (start + nb_subs + 1));
    if (main_command == NULL) {
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