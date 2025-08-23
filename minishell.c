#define _POSIX_C_SOURCE 200809L
#include <asm-generic/signal-defs.h>
#include <ctype.h>
#include <errno.h>
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
#define BACK_PROCESS_SIZE 10 /* size of background process struct array */
char line[NL]; /* command input buffer */

/* global background processes*/
struct BCKProcess {
  int pid;
  int job_number;
  char command[NL];
  int message_size;
};

/* global array of background structs */
struct BCKProcess *bg_processes, *temp;
int buffer_size;

/* helper function to locate index to 
struct with command associated with pid 
*/
int find_index(int pid) {
  for (int i = 0; i < buffer_size; i++) {
    if (bg_processes[i].pid == pid) {
      return i;
    }
  }
  return -1;
}

/* helper function to resize background struct */
int resize_BCKProcess() {
  temp = realloc(bg_processes, buffer_size * 2);
  if (temp != NULL) {
    bg_processes = temp;
    buffer_size = buffer_size * 2;
  } else {
    perror("realloc ERROR");
    return -1;
  }
  return 0;
}

/* helper function to store background process information */
int insert_process(int fork, int job, char *line) {
  printf("[%d] %d\n", job, fork);
  bg_processes[job].pid = fork;
  bg_processes[job].job_number = job;

  /* create custom DONE message */
  snprintf(bg_processes[job].command, sizeof(bg_processes[job].command),
           "[%d]+ Done %s\n", job, line);
  bg_processes[job].message_size = strlen(bg_processes[job].command);
  int update_job = job + 1;
  if (update_job == buffer_size) {
    if (resize_BCKProcess() == -1) {
      return -1;
    }
  }
  return update_job;
}

/* helper function to remove background & and surrounding whitespace */
void remove_amper(char *string) {
  size_t n = strlen(string);
  ssize_t i = (ssize_t)n - 1;

  while (i >= 0 && isspace((unsigned char)string[i])) {
    string[i] = '\0';
    i--;
  }
  if (i >= 0 && string[i] == '&') {
    string[i] = '\0';
    i--;
  }
  while (i >= 0 && isspace((unsigned char)string[i])) {
    string[i] = '\0';
    i--;
  }
}

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
    /* Remove outer single/double quotations from pathway*/
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
    /* Instruct waitpid to continue when "Interrupted with system call" */
    if (errno == EINTR){
      continue;
    }

    /* Is caught process background_process?*/
    int index = find_index(pid);
    if (index == -1){
      continue;
    }

    /* Child process exited normally or by signal */
    if (WIFEXITED(status) == true){
      /* write DONE message */
      write(STDOUT_FILENO, bg_processes[index].command, bg_processes[index].message_size);
    } else if (WIFSIGNALED(status) == true){
      perror("Child Process Ended by Signal");
    }
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
  int job_number = 1;      /* arbiturary job number*/

  /* Initalise global variables & data structures */
  bg_processes = (struct BCKProcess *)malloc(BACK_PROCESS_SIZE * sizeof(struct BCKProcess));
  if (bg_processes == NULL) {
    perror("malloc ERROR");
    return -1;
  }
  buffer_size = BACK_PROCESS_SIZE;

  /* initialise sigaction to custom sigchild signal handler 
  set empty signal mask of calling process
  SA_NOCLDSTOP to remove stop/pauses, only consider termination (SIGCHLD)
  */
  struct sigaction sigact_child;
  sigact_child.sa_handler = &sigchild;
  sigemptyset(&(sigact_child.sa_mask));
  sigact_child.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sigact_child, NULL);

  while (1) { /* do Forever */
    char saved_line[NL];
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);
    strcpy(saved_line, line);

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
      remove_amper(saved_line);
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
          /* Add background process to process array, update job number
          and immediately proceed to next command
          */
          if (background == true) {
            job_number = insert_process(frkRtnVal, job_number, saved_line);
          } else {
            /* wait for current foreground process to complete*/
            int status;
            waitpid(frkRtnVal, &status, 0);
          }
        }
      } /* switch */
    } /* else */
  } /* while */
} /* main */
