#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
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