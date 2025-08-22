#define _POSIX_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t sighup_receive = 0;
volatile sig_atomic_t sigint_receive = 0;

// custom function for HUP signal
void sighup(int signum) { sighup_receive = 1; }

// custom function for INT signal
void sigint(int signum) { sigint_receive = 1; }

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
  long int i = 0;
  char buffer[16];
  // loop printing even numbers "n" times
  while (i < n) {
    if (sighup_receive) {
      write(STDERR_FILENO, "Ouch!\n", 6);
      sighup_receive = 0;
    } else if (sigint_receive) {
      write(STDERR_FILENO, "Yeah!\n", 6);
      sigint_receive = 0;
    }
    snprintf(buffer, sizeof(buffer), "%d\n", even);
    write(STDERR_FILENO, buffer, strlen(buffer));
    even = even + 2;
    if (i < n - 1) {
      sleep(5);
    }
    i++;
  }
  return 0;
}
