#define _POSIX_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

// custom function for HUP signal
void sighup(int signum) {
  char *hup_message = "Ouch!\n";
  write(STDERR_FILENO, hup_message, strlen(hup_message));
  return;
}

// custom function for INT signal
void sigint(int signum) {
  char *int_message = "Yeah!\n";
  write(STDERR_FILENO, int_message, strlen(int_message));
  return;
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

  // change SIGHUP/SIGINT to custom behaviour.
  sigaction(SIGHUP, &sa_hup, NULL);
  sigaction(SIGINT, &sa_int, NULL);

  char *endptr;

  if (argc != 2){
    return -1;
  }
  
  // convert command input into long int
  long int n = strtol(argv[1], &endptr, 10);

  int even;
  // loop printing even numbers "n" times
  for (long int i = 0; i < n; i++) {
    printf("%d\n", even);
    even = even + 2;
    if (i < n-1){
      sleep(5);
    }
  } 
  return 0;
}
