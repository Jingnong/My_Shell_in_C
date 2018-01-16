/*  File name: util.c
 *  Project name: project1
 *  Author: Xintong Bao, Jingnong Wang
 *  Date: 04/09/2017
 */

#include <sys/resource.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

#define RC_CHECK(s) if(!(s)) run_child_error();

void run_child_error();

/* Function name: tokenize
 * Description: split a data buffer by spaces.
 * Parameters:
 *   buffer: pointer to the data buffer.
 *   argv: pointer to the output array, end up with NULL.
 *   maxargslen: maximum length of the output array.
 * Return:
 *   the number of arguments in the data buffer.
 */
int tokenize(char *buffer, char *argv[], int maxargs)
{
  int n = 0; // count the atguments in the data buffer
  
  /* skip spaces at the beginning, point to the head of the first word */
  while(*buffer && isspace(*buffer))
    buffer++;

  /* add non-space words in args to buffer */
  while(*buffer && n < maxargs)
  { argv[n++] = buffer; // point to the head of the first word

    while(*buffer && !isspace(*buffer))
      buffer++; // skip the remainning part of the word

    while(*buffer && isspace(*buffer))
      /* Mark end of token */
      *(buffer++) = 0; // mark the end of the token
  }
  
  argv[n] = NULL; // set the end of the word to NULL
  return n; // return number of the arguments in the data buffer
}

/* Function name: run_child
 * Description: Spawn a child process.
 * Parameters:
 *   progname: program name.
 *   argv: array of arguments. First element is program name, last element is NULL.
 *   child_stdin: file descriptor to be provided to child as standard input.
 *   child_stdout: file descriptor to be provided to child as standard output.
 *   child_stderr: file descriptor to be provided to child as standard error.
 * Return:
 *   PID of the child or -1 if fork() returned an error.
 * Error handling:
 *   If fork() returns an error, -1 is returned.
 *   For errors which happen in the child process, an error message is printed to stderr,
 *   and the child exits with a non-zero return value.
 */
pid_t run_child(char *progname, char *argv[], int child_stdin, int child_stdout, int child_stderr)
{
  pid_t child; // pid_t : int.
  struct rlimit lim; // resource limit of the process.
  int max;
  
  /* fork() return child process id if in parent process, return 0 if in child process, return -1 on error */
  /* 2 error types: reach the limit of number of processes; lack of memory */
  if((child = fork()))
  { /* in parent or on error */
    return child;
  }

  /* Set up file descriptors */
  
  /* First, duplicate the provided file descriptors to 0, 1, 2 */
  if(child_stdout == STDIN_FILENO)
  { /* Move stdout out of the way of stdin */
    child_stdout = dup(child_stdout);
    RC_CHECK(child_stdout >= 0); // dup() return the descriptor of new file, or -1 on error.
  }

  while(child_stderr == STDIN_FILENO || child_stderr == STDOUT_FILENO)
  { /* Move stderr out of the way of stdin/stdout */
    child_stderr = dup(child_stderr);
    RC_CHECK(child_stderr >= 0);
  }
  
  /* int dup2(int oldfd, int newfd) */
  child_stdin = dup2(child_stdin,STDIN_FILENO);  // Force stdin onto 0
  RC_CHECK(child_stdin == STDIN_FILENO);
  child_stdout = dup2(child_stdout,STDOUT_FILENO);  // Force stdout onto 1
  RC_CHECK(child_stdout == STDOUT_FILENO);
  child_stderr = dup2(child_stderr,STDERR_FILENO);  // Force stderr onto 2
  RC_CHECK(child_stderr == STDERR_FILENO);

  RC_CHECK(getrlimit(RLIMIT_NOFILE,&lim) >= 0); // Get max file number
  max = (int)lim.rlim_cur;

  /* Close all other open files when execute the program */
  for(int fd = STDERR_FILENO + 1; fd < max; fd++)
  { int code = fcntl(fd,F_GETFD); // get the close-on-exec flag. if zero, file will not be closed when exec().

    if(!code&FD_CLOEXEC) // if the file is not open, FD_CLOEXEC looks set
    { if(fcntl(fd,F_SETFD,code|FD_CLOEXEC) < 0)
        perror("run_child"); // Report errors, but proceed
    }
  }

  /* Execute the program */
  execvp(progname,argv);
  run_child_error();
  exit(1);
}

/* If an error occurs, run perror and exit */
void run_child_error()
{ perror("run_child");

  if(errno & 255) // errno: an int, the last error number.
    exit(errno); /* Normally reasonable */
  else
    exit(1); /* Always return someting negative */
}
