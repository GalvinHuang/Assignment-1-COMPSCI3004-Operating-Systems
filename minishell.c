#define _POSIX_C_SOURCE 200809L
#include <asm-generic/signal-defs.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */
char line[NL]; /* command input buffer */

/* shell prompt */
void prompt(void) { fflush(stdout); }

/* helper function to proces cd command*/
void process_cd(char *token[], int size) {
  int length = strlen(token[1]);
  if (token[1] == NULL || strcmp(token[1], "~") == 0 ||
      strcmp(token[1], "$HOME") == 0) {
    /* proceed to home directory (cd , cd ~, cd $HOME) */
    if (chdir(getenv("HOME")) != 0) {
      perror("cd $HOME ERROR");
    }
  } else if (token[1][0] == '~') {
    /* proceed to home directory + PATH*/
    if (chdir(getenv("HOME")) != 0) {
      perror("cd $HOME ERROR");
    }
    memmove(token[1], token[1] + 2, strlen(token[1]) + 2 - 1);  // Remove ~/
    printf("New Directory %s\n", token[1]);
    if (chdir(token[1]) != 0) {
      perror("cd ERROR");
    }
  } else if ((token[1][0] == '\'' && token[1][length - 1] == '\'') ||
             (token[1][0] == '"' && token[1][length - 1] == '"')) {
    char *pathway = malloc(length - 1 * sizeof(char));
    if (pathway == NULL) {
      perror("malloc ERROR");
      return;
    }
    strncpy(pathway, token[1] + 1, length - 2);
    pathway[length - 2] = '\0';
    if (chdir(pathway) != 0) {
      perror("cd ERROR");
    }
    free(pathway);
  } else {
    /* proceess directory */
    if (chdir(token[1]) != 0) {
      perror("cd ERROR");
    }
  }
  return;
}

/* waipid waits for any termination of child background processes (-1)
if status information for at least one process (child) isn't available,
waitpid() return 0 (WNOHANG)
*/
void sigchild(int signum) {
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
  }
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[]) {
  int frkRtnVal;           /* value returned by fork sys call */
  char *v[NV];             /* array of pointers to command line tokens */
  char *sep = " \t\n";     /* command line token separators    */
  int i;                   /* parse index */
  bool background = false; /* boolean to indicate command to be run in background*/
  int job_number = 0;      /* arbiturary job number*/

  /* initialise sigaction to custom sigchild signal handler 
  set empty signal mask of calling process
  SA_NOCLDSTOP to remove stop/pauses, only consider termination (SIGCHLD)
  */
  struct sigaction sigact_child;
  sigact_child.sa_handler = &sigchild;
  sigemptyset(&(sigact_child.sa_mask));
  sigact_child.sa_flags = SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sigact_child, NULL);

  while (1) { /* do Forever */
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);

    if (feof(stdin)) { /* non-zero on EOF  */
      exit(0);
    }
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\000') {
      continue; /* to prompt */
    }

    int size = 1;
    v[0] = strtok(line, sep);
    for (i = 1; i < NV; i++) {
      v[i] = strtok(NULL, sep);
      if (v[i] == NULL) {
        break;
      }
      size++;
    }

    /* detect & symbol for background mode */
    background = false;
    if (strcmp(v[size - 1], "&") == 0) {
      v[size - 1] = NULL;
      background = true;
    }

    /* detect cd command for proper execution */
    if (strcmp(v[0], "cd") == 0) {
      process_cd(v, size);
    } else {
      /* fork a child process to exec the command in v[0] */
      switch (frkRtnVal = fork()) {
        case -1: /* fork returns error to parent process */
        {
          perror("fork ERROR");  // fork perror message
          break;
        }
        case 0: /* code executed only by child process */
        {
          execvp(v[0], v);
          /* Custom perror message to include command */
          char error_message[NL + 15];
          strcat(error_message, v[0]);
          strcat(error_message, " command ERROR");
          perror(error_message);
          exit(EXIT_FAILURE);
        }
        default: /* code executed only by parent process */
        {
          if (background == true) {
            printf("[%d] %d\n", job_number, getpid());
            job_number++;
          } else {
            wait(0);
          }
        }
      } /* switch */
    } /* else */
  } /* while */
} /* main */
