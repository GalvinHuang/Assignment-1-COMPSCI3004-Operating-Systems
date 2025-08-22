/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/
#define _POSIX_SOURCE
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

#define NV 20		              /* max number of command tokens */
#define NL 100	              /* input buffer size */
#define BACK_PROCESS_SIZE 10  /* size of background process struct array */
char line[NL];	/* command input buffer */


/* global background processes*/
struct BCKProcess {
  int pid;
  int job_number;
  char *command;
};

/* global array of background structs */
struct BCKProcess *background_processes, *temp;
int buffer_size;

/* global pipe to retrive pid of completed background processes*/
int pipefd[2];

/* helper function to locate index to 
struct with command associated with pid 
*/
int find_index(int pid) { 
  for (int i = 0; i < buffer_size; i++){
    if (background_processes[i].pid == pid){
      return i;
    }
  }
  return -1;
}

/* helper function to resize background struct */
int resize_BCKProcess() {
  temp = realloc(background_processes, buffer_size * 2);
  if (temp != NULL){
    background_processes = temp;
    buffer_size = buffer_size * 2;
  } else {
    perror("realloc ERROR");
    return -1;
    }
  return 0;
}

/* helper function to record background process */
int insert_process(int fork, int job, char *line){
  printf("[%d] %d\n", job, fork);
  background_processes[job].pid = fork;
  background_processes[job].job_number = job;
  background_processes[job].command = line;
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
void prompt(void){
  // ## REMOVE THIS 'fprintf' STATEMENT BEFORE SUBMISSION
  fprintf(stdout, "\n msh> ");
  fflush(stdout);
}

/* waipid waits for any termination of child background processes (-1)
if status information for at least one process (child) isn't available,
waitpid() return 0 (WNOHANG)
*/
void sigchild(int signum) {
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0){
    /* Instruct waitpid to continue when 
    "Interrupted with system call"
    */
    if (errno == EINTR){
      continue;
    }
    
    /* Is caught process background_process?*/
    int index = find_index(pid);
    if (index == -1){
      continue;
    }

    /* Child process exited normally / by signal */
    if (WIFEXITED(status) == true){
      write(pipefd[1], &pid, sizeof(pid));
    } else if (WIFSIGNALED(status) == true){
      perror("Child Process Ended by Signal");
      //printf("Signal Value: %d\n", WTERMSIG(status));
    }
  }
  return;
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
  int frkRtnVal;	          /* value returned by fork sys call */
  char *v[NV];	            /* array of pointers to command line tokens */
  char *sep = " \t\n";      /* command line token separators */
  int i;		                /* parse index */
  bool background = false;  /* boolean to indicate command to be run in background */
  int job_number = 0;       /* arbiturary job number */

  /* Initalise global variables & data structures */
  background_processes = (struct BCKProcess *)malloc(BACK_PROCESS_SIZE * sizeof(struct BCKProcess));
  if (background_processes == NULL) {
    perror("malloc ERROR");
    return -1;
  }
  buffer_size = BACK_PROCESS_SIZE;
  pipe(pipefd);

  /* initialise sigaction to custom sigchild signal handler 
  set empty signal mask of calling process
  SA_NOCLDSTOP to remove stop/pauses, only consider termination (SIGCHLD)
  */
  struct sigaction sigact_child;
  sigact_child.sa_handler = &sigchild;
  sigemptyset(&(sigact_child.sa_mask));
  sigact_child.sa_flags = SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sigact_child, NULL);

  while (1) {			/* do Forever */
    char saved_line[NL]; /* Saved line */
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);
    
    strcpy(saved_line, line);

    if (feof(stdin)) {		/* non-zero on EOF  */
      //fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),feof(stdin), ferror(stdin));
      /* Free memory and close parent pipe */
      free(background_processes);
      close(pipefd[0]);
      exit(0);
    }
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\000'){
      continue;			/* to prompt */
    }

    int size = 0;
    v[0] = strtok(line, sep);
    for (i = 1; i < NV; i++) {
      v[i] = strtok(NULL, sep);
      size++;
      if (v[i] == NULL) {
        break;
      }
    }
    /* assert i is number of tokens + 1 */

    /* detect & symbol for background mode */
    background = false;
    if (strcmp(v[size - 1], "&") == 0) {
      v[size - 1] = '\0';
      remove_amper(saved_line);
      background = true;
    }

    /* detect cd command for proper execution */
    if (strcmp(v[0],"cd") == 0){
      /* change directory command */
      if (v[1] == NULL){
        /* Stay in current directory (no specified path) */
        chdir("~");
      } else {
        /* Go to specified directory (specified path)*/
        if (chdir(v[1]) != 0){
          perror("cd command ERROR"); // cd perror message
        }
      }
    } else {
      /* fork a child process to exec the command in v[0] */
      switch (frkRtnVal = fork()) {
        case -1:			/* fork returns error to parent process */
        {
          perror("fork ERROR"); // fork perror message
          break;
        }
        case 0:			/* code executed only by child process */
        {
          /* Child close read/write end of copy of global SIGCHILD pipe*/
          close(pipefd[0]);
          close(pipefd[1]);
          execvp(v[0], v);
          /* Custom perror message to include command */
          char error_message[NL + 15];
          strcat(error_message, v[0]);
          strcat(error_message," command ERROR");
          perror(error_message);
          exit(EXIT_FAILURE);
        }
        default:			/* code executed only by parent process */
        {
          /* Parent close write end of pipe */
          close(pipefd[1]);
          if (background == true){
            job_number = insert_process(frkRtnVal, job_number, saved_line);
          }
          /* Print DONE message for all completed background processes (saved pid)*/
          pid_t pid;
          while (read(pipefd[0], &pid, sizeof(pid)) > 0) {
            int index = find_index(pid);
            if (index != -1){
              char done_message[NL + 13];
              snprintf(done_message, sizeof(done_message), "[%d] Done %s\n",
              background_processes[index].job_number,
              background_processes[index].command);
            }
          }
        }
      }				/* switch */
    }      /* else */
  }				/* while */
}				/* main */
