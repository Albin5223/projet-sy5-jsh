#include "substitution_process.h"
#include "utils.h"
#include "redirection.h"
#include "internalCommand.h"
#include "jobs.h"
#include "pipe.h"

/**
 * @param command_args tableau de commandes
 * @return le nombre de subs '<(' suivi de ')' dans une chaine de caractères donnée
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
 * @param command_args
 * @param i
 * @return retourne le nombre de '<(' suivi ')', lorsqu'on parcourt le tableau de commandes
*/
int ok_subs(char **command_args, int i){
    if (strcmp(command_args[i], "<(") == 0){
        while(1){
            char *tmp = command_args[i++];
            if (tmp == NULL) break;
            if (strcmp(tmp, ")") == 0) return true;
        }
    }
    return false;
}

int exist_fic(char *file){
    if (file == NULL) return 0;
    int fd = open(file, O_RDONLY);
    return (fd != -1) ? fd : 1;
}

/**
 * @param command_args
 * @param nb_substitution
 * @return un tableau de la première occurence de ce qui se trouve à l'intérieur de : '<(' et ')'
*/
char **get_substitution_process(char **command_args) {
    int diff = 0;
    int size = 0;
    int i = 0;
    bool in = false;
    bool out = false;
    //cat -n <( echo "Header" ) <( echo "Footer" ) <( diff <( echo "aaaz" ) <( echo "zzz" ) )
    while (!out) {
        if (command_args[i] == NULL) break;
        
        if (in){
            if (strcmp(command_args[i], "<(") == 0) diff++;
            if (strcmp(command_args[i], ")") == 0){
                if (diff == 1) {
                    in = false;
                    out = true;
                    diff--;
                }
                else diff--;
            }
            if (in) size++;
        } else {
            if (strcmp(command_args[i], "<(") == 0) {
                in = true;
                diff++;
            }
        }
        i++;
    }

    char **tab_subs = malloc(sizeof(char*) * (size + 1));
    if (tab_subs == NULL) exit(1);

    for (int j = 0; j < size; j++) {
        tab_subs[j] = command_args[1 + j];
    }
    tab_subs[size] = NULL;

    return tab_subs;
}

/**
 * Retourne le tableau de commande avant le premier '<('
 * @param command_args
 * @return le tableau de commande avant le premier '<('
*/
char **get_main_command(char **command_args){
    int size = 0;

    int fd;
    while(1){
        if (command_args[size] == NULL) break;
        if (strcmp(command_args[size], "<(") == 0) break;
        if ((fd = exist_fic(command_args[size])) != 1){
            close(fd);
            break;
        }
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

/**
 * @param command_args
 * @return le nombre d'arguments dans une chaine de caractères donnée
*/
int nb_args(char **command_args){
    int size = 0;
    int diff = 0;
    int i = 0;
    int fd;
    bool in = false;

    while (1) {
        if (command_args[i] == NULL) break;
        
        if (in){
            if (strcmp(command_args[i], "<(") == 0) diff++;
            if (strcmp(command_args[i], ")") == 0){
                if (diff == 1) {
                    in = false;
                    diff--;
                }
                else diff--;
            }
        } else {
            if (strcmp(command_args[i], "<(") == 0) {
                in = true;
                diff++;
            }
            if ((fd = exist_fic(command_args[i])) != 1){
                close(fd);
            }
            size++;
        }
        i++;
    }

    return size;
}


/**
 * cat -n <( echo "Header" ) <( ps a ) <( echo "Footer" ) 
 * cat -n <( echo "Header" ) fichierTest <( echo "Footer" ) 
 * cat -n <( echo "Header" ) <( echo "Footer" ) <( diff <( echo "aaaz" ) <( echo "zzz" ) )
 * cat <( echo "Header" | tr 'a-z' 'A-Z' ) <( ps a ) <( echo "Footer" | tr 'a-z' 'A-Z' )
 * diff <( echo "aaaz" ) <( echo "zzz" ) &
*/

int execute_substitution_process(char **command_args, int nb_substitution) {
    if (isRedirection(command_args) != -1) return 1;

    int ret = 0;
    char **main_command = get_main_command(command_args);

    char ***tab = malloc(sizeof(char**) * (nb_substitution + 1));
    if (tab == NULL) exit(1);
    tab[nb_substitution] = NULL;

    command_args += len(main_command);
    int size = nb_args(command_args); 
    char reference[size][256];

    int pipefd[nb_substitution][2];
    int j = 0; // pour parcourir le tableau : tab et pipefd

    for (int i = 0; i < size; i++){

        if (strcmp(command_args[0], "<(") != 0){
            snprintf(reference[i], sizeof(reference[i]), "%s", command_args[0]);
            command_args++;
            continue;
        }

        if (pipe(pipefd[j]) == -1) {
            ret = 1;
            perror("substitution_process");
            goto cleanup;
        }

        tab[j] = get_substitution_process(command_args);
        pid_t pid = fork();

        if (pid == -1) {
            ret = 1;
            perror("substitution_process");
            goto cleanup;
        }
  
        if (pid == 0) {
            dup2(pipefd[j][1], STDOUT_FILENO);

            if (isInternalCommand(tab[j]) == 1){
                ret = executeInternalCommand(tab[j]);
                exit(ret);
            }

            if (isPipe(tab[j]) == 1){
                ret = add_job_command_with_pipe(tab[j], false);
                exit(ret);
            }

            if (nb_subs(tab[j]) > 0){
                ret = execute_substitution_process(tab[j], nb_subs(tab[j]));
                exit(ret);
            }

            execvp(tab[j][0], tab[j]);
            exit(EXIT_FAILURE);
        } else {
            close(pipefd[j][1]);
             
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                ret = 1;
                goto cleanup;
            }

            snprintf(reference[i], sizeof(reference[i]), "/dev/fd/%d", pipefd[j][0]);
            
            command_args += len(tab[j]) + 2;    // afin de sauter : <( et )
        }
    }

    for (int i = 0; i < nb_substitution; i++){
        wait(NULL);
    }

    int start = len(main_command);
    main_command = realloc(main_command, sizeof(char*) * (start + size + 1));
    if (main_command == NULL) {
        ret = 1;
        perror("substitution_process");
        goto cleanup;
    }
    main_command[start + size] = NULL;

    for (int i = 0; i < size; i++) {
        main_command[start + i] = reference[i];
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

    for (int i = 0; i < nb_substitution; i++){
        close(pipefd[i][0]);
    }

    wait(NULL);

cleanup:
    for (int i = 0; i < nb_substitution; i++) free(tab[i]);
    free(tab);
    return ret;
}