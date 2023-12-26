#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "jobs.h"
#include "utils.h"
#include "colors.h"
#include "pipe.h"
#include "redirection.h"
#include "internalCommand.h"
#include "kill.h"

/**
 * C'est fonction va récupérer tous les fils du processus pid
 * Retourne un tableau d'entier contenant les pid des fils qui doit être free
*/
int *getChildOfPid(int pid){
    char path[100];
    sprintf(path,"/proc/%d/task/%d/children",pid,pid);

    int fd = open(path,O_RDONLY);
    if(fd == -1){
        return NULL;
    }
    int *children = malloc(sizeof(int)*100);
    char buffer[4096];
    int size = read(fd,buffer,4096);
    if(size == -1){
        close(fd);
        return NULL;
    }

    int index = 0;
    const char delim[2] = " ";
    char *token = strtok(buffer,delim);
    while(token != NULL){
        children[index] = atoi(token);
        index++;
        token = strtok(NULL,delim);
    }
    children[index] = -1;
    close(fd);
    return children;
}

char *convertSymboleOfState(char *c){
    if(strcmp(c,"R") == 0){
        return "Running";
    }
    if(strcmp(c,"S") == 0){
        return "Sleeping";
    }
    if(strcmp(c,"D") == 0){
        return "Waiting";
    }
    if(strcmp(c,"Z") == 0){
        return "Zombie";
    }
    if(strcmp(c,"T") == 0){
        return "Stopped";
    }
    return "Unknown";
}

char *getInfoOfPid(int pid){
    char path[100];
    sprintf(path,"/proc/%d/stat",pid);
    int fd = open(path,O_RDONLY);
    if(fd == -1){
        return NULL;
    }
    char buffer[4096];
    int size = read(fd,buffer,4096);
    if(size == -1){
        close(fd);
        return NULL;
    }
    
    char *info = malloc(sizeof(char)*100);

    int index = 0;
    char pidChar[15];
    char name[100];
    char state[100];

    int numberOfSpace = 0;
    for(int i = 0; i < size; i++){
        if(buffer[i] == ' '){

            numberOfSpace++;
            index=0;
            if(numberOfSpace == 3){
                break;
            }
            
            continue;
        }

        if(numberOfSpace == 0){
            pidChar[index] = buffer[i];
            index++;
            pidChar[index] = '\0';
        }
        else if(numberOfSpace == 1){
            name[index] = buffer[i];
            index++;
            name[index] = '\0';
        }
        else if(numberOfSpace == 2){
            
            state[index] = buffer[i];
            index++;
            state[index] = '\0';
        }
    }
    int n = strlen(name);
    name[n-1] = '\0';
    

    char *newState = convertSymboleOfState(state);
    sprintf(info,"%s %s %s",pidChar,newState,name+1);

    return info;
}

bool hasChild(int pid){
    int *children = getChildOfPid(pid);
    if(children == NULL){
        return false;
    }
    int index = 0;
    while (children[index] != -1){
        index++;
    }
    free(children);
    return index > 0;
}

void printChildWithIndent(char *info,int std,int indent){
    char *space = "     ";
    for(int i = 0; i < indent; i++){
        dprintf(std,"%s",space);
    }
    dprintf(std,"%s\n",info);
}



void printChildOfJob(int pid,int std,int indent){
    if(!hasChild(pid)){
        return;
    }
    int *children = getChildOfPid(pid);
    
    int index = 0;
    while (children[index] != -1){
        char *info = getInfoOfPid(children[index]);
        printChildWithIndent(info,std,indent);
        free(info);
        if(hasChild(children[index])){
            printChildOfJob(children[index],std,indent+1);
        }
        index++;
    }
    free(children);
    return;
}


int executeinternalJobs(char **commandeArgs){
    char pourcentage = '%';
    int size = len(commandeArgs);

    if (size == 1){
        print_all_jobs(false);
        return 0;
    }
    if (size == 2){
        if(strcmp(commandeArgs[1],"-t")==0){
            print_all_jobs(true);
            return 0;
        }
        if(start_with_char_then_digits(commandeArgs[1],pourcentage)){
            int id = atoi(commandeArgs[1]+1);
            if (id == 0){
                dprintf(STDERR_FILENO,"Erreur: jobs [-t] [%cjob], job > 0 \n",pourcentage);
                return -1;
            }
            int pid = get_pid_by_id(id);
            return print_job_with_pid(pid,false,1);
        }
        return -1;
    }
    else{
        if (size == 3){
            if (strcmp(commandeArgs[1],"-t")==0){
                if(start_with_char_then_digits(commandeArgs[2],pourcentage)){
                    int id = atoi(commandeArgs[2]+1);
                    if (id == 0){
                        dprintf(STDERR_FILENO,"Erreur: jobs [-t] [%cjob], job > 0 \n",pourcentage);
                        return -1;
                    }
                    int pid = get_pid_by_id(id);
                    return print_job_with_pid(pid,true,1);
                }
            }
        }
    }
    dprintf(STDERR_FILENO,"Erreur: jobs [-t] [%cjob] \n",pourcentage);
    return -1;
}