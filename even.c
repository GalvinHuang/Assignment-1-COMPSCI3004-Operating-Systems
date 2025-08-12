#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

// custom function for HUP signal
void sighup() {
  char *hup_message = "Ouch!";
  write(STDERR_FILENO, hup_message, strlen(hup_message));
  return;
}

// custom function for INT signal
void sigint() {
  char *int_message = "Yeah!";
  write(STDERR_FILENO, int_message, strlen(int_message));
  return;
}

int main(int argc, char *argv[]) {
  signal(SIGHUP, sighup);
  signal(SIGINT, sigint);
  int n;

  // convert command input into int
  if (argc == 2){
    sscanf(argv[1], "%d", &n);
  }

  int even = 0;
  // loop printing even numbers "n" times
  for (int i = 0; i < n; i++) {
    printf("%d\n", even);
    even = even + 2;
    sleep(5);
  } 

  return 0;
}
