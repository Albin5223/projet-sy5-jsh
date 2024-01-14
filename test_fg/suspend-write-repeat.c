#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

int my_usleep(useconds_t microseconds);
int main(int argc, char ** argv) {
  unsigned int reps;
  char * msg;
  if (argc != 3) {
    fprintf(stderr, "Usage: suspend-write-repeat MESSAGE REPETITIONS\n");
    exit(1);
  }
  if (sscanf(argv[2], "%u", &reps) != 1) {
    fprintf(stderr, "Invalid repetition count\n");
  }
  msg = argv[1];

  for (unsigned int i=0; i < reps; i++) {
    sleep(2);
    killpg(getpgid(0), SIGSTOP);
    sleep(2);
    sleep(2);
    printf("%s %u\n", msg, i);
    fflush(stdout);
  }
  sleep(2);

  struct sigaction act_ign = {0};
  act_ign.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &act_ign, NULL);
  killpg(getpgid(0), SIGPIPE);
}