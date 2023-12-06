#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    fork();
    sleep(100);
    return EXIT_SUCCESS;
}