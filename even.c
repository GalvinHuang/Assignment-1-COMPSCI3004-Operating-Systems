#include <stdio.h>
#include <unistd.h>
#include <signal.h>

// custom function for HUP signal
void sighup() {
  printf("Ouch!");
  return;
}

// custom function for INT signal
void sigint() {
  printf("Yeah!");
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
  for (int i = 0; i < n; i++) {
    printf("%d\n", even);
    even = even + 2;
    sleep(5);
  } 

  return 0;
}
