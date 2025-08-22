#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// custom function for HUP signal
void sighup(int signum) { 
    write(STDOUT_FILENO, "Ouch!\n", 6);
}

// custom function for INT signal
void sigint(int signum) { 
    write(STDOUT_FILENO, "Yeah!\n", 6); 
}

int main(int argc, char *argv[]) {
  // initialise sigaction struct.
  struct sigaction sa_hup, sa_int;

  // pointer to custom function.
  sa_hup.sa_handler = &sighup;
  sa_int.sa_handler = &sigint;

  // set empty signal mask of calling process.
  sigemptyset(&(sa_hup.sa_mask));
  sigemptyset(&(sa_int.sa_mask));

  // set empty flags.
  sa_hup.sa_flags = 0;
  sa_int.sa_flags = 0;

  // change SIGHUP/SIGINT to custom behaviour.
  sigaction(SIGHUP, &sa_hup, NULL);
  sigaction(SIGINT, &sa_int, NULL);

  char *endptr;

  // convert command input into long int
  long int n = strtol(argv[1], &endptr, 10);
  int even = 0;

  // loop printing even numbers "n" times
  for (long int i = 0; i < n; i++){
    printf("%d\n", even);
    fflush(stdout); // Force stdout immediately?
    even = even + 2;
    sleep(5);
  }
  return 0;
}
