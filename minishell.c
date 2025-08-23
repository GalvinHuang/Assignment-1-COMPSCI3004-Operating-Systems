/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/
#define _POSIX_C_SOURCE 199309L
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <asm-generic/signal-defs.h>

#define NV 20			/* max number of command tokens */
#define NL 100			/* input buffer size */
char            line[NL];	/* command input buffer */

/*
	shell prompt
 */

void prompt(void)
{
  fflush(stdout);
}

/* helper function to proces cd command*/
void process_cd(char *token[], int size) {
  if (token[1] == NULL || strcmp(token[1], "~") == 0 || strcmp(token[1], "$HOME") == 0) {
    /* proceed to home directory (cd , cd ~, cd $HOME) */
    if (chdir(getenv("HOME")) !=0){
      perror("cd $HOME ERROR");
    }
  } else if (token[1][0] == '~') {
    /* proceed to home directory + PATH*/
    if (chdir(getenv("HOME")) !=0){
      perror("cd $HOME ERROR");
    }
    memmove(token[1], token[1] + 1, strlen(token[1]) + 1 - 1); // Remove ~/
    if (chdir(token[1]) !=0){
      perror("cd ERROR");
    }
  } else {
    /* proceess directory */
    if (chdir(token[1]) !=0){
      perror("cd ERROR");
    }
  }
  return;
}

void sigchild(int signum) { 
  int status;
  pid_t pid;
  /* waitpid watiing for any background processes (-1)
  With WNOHANG was given, if status information is not available for at least one process (child)
  waitpid() returns a value of 0.
  */
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0){}
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
  int             frkRtnVal;	    /* value returned by fork sys call */
  //int             wpid;		        /* value returned by wait */
  char           *v[NV];	        /* array of pointers to command line tokens */
  char           *sep = " \t\n";  /* command line token separators    */
  int             i;		          /* parse index */
  bool       background = false;  /* boolean to indicate command to be run in background*/
  int        job_number = 0;      /* arbiturary job number*/

  /* prompt for and process one command line at a time  */
  
  /* sigaction struct linked to custom sigchild handler
  sigemptyset to specify no other signals are blocked during custom handler
  SA_NOCLDSTOP to remove stop/pauses, only consider termination.
  SA_RESTART to restart interrupted library functions.
  */
  struct sigaction sigact;
  sigact.sa_handler = sigchild;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sigact, NULL);

  while (1) {			/* do Forever */
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);

    if (feof(stdin)) {		/* non-zero on EOF  */

      fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),
	      feof(stdin), ferror(stdin));
      exit(0);
    }
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\000'){
      continue;			/* to prompt */
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
    /* assert i is number of tokens + 1 */

    /* detect & symbol for background mode */
    background = false;
    if (strcmp(v[size - 1], "&") == 0) {
      v[size - 1] = NULL;
      background = true;
    }

    /* detect cd command for proper execution */
    if (strcmp(v[0],"cd") == 0){
      process_cd(v, size);
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
            printf("[%d] %d\n", job_number, getpid());
            job_number++;
          } else {
            wait(0);
          }
        }
      }				/* switch */
    }      /* else */
  }				/* while */
}				/* main */
