/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20			/* max number of command tokens */
#define NL 100			/* input buffer size */
char            line[NL];	/* command input buffer */


/*
	shell prompt
 */

void prompt(void)
{
  // ## REMOVE THIS 'fprintf' STATEMENT BEFORE SUBMISSION
  fprintf(stdout, "\n msh> ");
  fflush(stdout);
}


/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
   int             frkRtnVal;	    /* value returned by fork sys call */
   int             wpid;		        /* value returned by wait */
  char           *v[NV];	        /* array of pointers to command line tokens */
  char           *sep = " \t\n";  /* command line token separators    */
  int             i;		          /* parse index */

    /* prompt for and process one command line at a time  */

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
    if (strcmp(v[size - 1], "&") == 0) {
      printf("Amber Alert!\n");
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
          int status;
          waitpid(frkRtnVal, &status, WUNTRACED);
          printf("%s command Exit Code: %d\n", v[0], WEXITSTATUS(status));
          break;
        }

      }				/* switch */
    }      /* else */
  }				/* while */
}				/* main */
