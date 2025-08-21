/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/
#define _POSIX_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <asm-generic/signal-defs.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

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
int size;

/* helper function to locate index to 
struct with command associated with pid 
*/
int find_index(int pid) { 
  for (int i = 0; i < size; i++){
    if (background_processes[i].pid == pid){
      return i;
    }
  }
  return -1;
}

/* helper function to resize background struct */
int resize_BCKProcess(int job) {
  temp = realloc(background_processes, size * 2);
  if (temp != NULL){
    background_processes = temp;
    size = size * 2;
  } else {
    perror("realloc ERROR");
    return -1;
    }
  return 0;
}

/* shell prompt */
void prompt(void){fflush(stdout);}

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
    
    int index = find_index(pid);
    if (index == -1){
      char *index_message = "FAILED to locate command with pid. \n";
      return;
    }

    char done_message[NL + 13];
    done_message[0] = '[';
    done_message[1] = '\0';
    char job[16];

    snprintf(job, sizeof(job), "%d", background_processes[index].job_number);
    strcat(done_message, "] Done ");
    strcat(done_message, background_processes[index].command);
    strcat(done_message, "\n");

    /* Child process exited normally / by signal */
    if (WIFEXITED(status) == true){
      write(STDERR_FILENO, done_message, strlen(done_message));
    } else if (WIFSIGNALED(status) == true){
      perror("Child Process Ended by Signal");
      printf("Signal Value: %d\n", WTERMSIG(status));
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

  background_processes = (struct BCKProcess *)malloc(BACK_PROCESS_SIZE * sizeof(struct BCKProcess));
  if (background_processes == NULL) {
    perror("malloc ERROR");
    return -1;
  }
  size = BACK_PROCESS_SIZE;

  /* initialise sigaction to custom sigchild signal handler 
  set empty signal mask of calling process
  SA_NOCLDSTOP to remove stop/pauses, only consider termination (SIGCHLD)
  */
  struct sigaction sigact_child;
  sigact_child.sa_handler = &sigchild;
  sigemptyset(&(sigact_child.sa_mask));
  sigact_child.sa_flags = SA_NOCLDSTOP;

  while (1) {			/* do Forever */
    sigaction(SIGCHLD, &sigact_child, NULL);
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);

    if (feof(stdin)) {		/* non-zero on EOF  */
      //fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),feof(stdin), ferror(stdin));
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
      size--;
      background = true;
    }

    for (i = 0; i < size; i++){
      printf("WHY %s\n", v[i]);
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
          if (background == true){
            printf("[%d] %d\n", job_number, frkRtnVal);
            background_processes[job_number].pid = frkRtnVal;
            background_processes[job_number].job_number = job_number;
            background_processes[job_number].command = line;
            job_number++;
            if (job_number == size){
              if (resize_BCKProcess(job_number) == -1){
                return -1;
              }
            }
            break;
          } else {
            wait(0);
            break;
          }
        }
      }				/* switch */
    }      /* else */
  }				/* while */
  free(background_processes);
}				/* main */
