#ifndef JOBSCOMMAND_H
#define JOBSCOMMAND_H

#define Running "R"
#define Sleeping "S"
#define Uninterruptible Sleep "D"
#define Zombie "Z"
#define Stoppe "T"


int executeinternalJobs(char **args);
void printChildOfJob(int id,int std,int indent);


#endif