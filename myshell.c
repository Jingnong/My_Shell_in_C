/*  File name: myshell.c
 *  Project name: project1
 *  Author: Xintong Bao, Jingnong Wang
 *  Date: 04/09/2017
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "myshell.h"
#include "util.h"

/* Max. length of input */
#define MAXLEN 1024

#define BUFSIZE 1024
#define CMDNUMBER 100

/*
 * Implementation of a shell. Command-line input is grabbed with fgets, and
 * parsed by handle_line. handle_line parses input into 'command chunks', which
 * each represent a single command (with arguments) in the pipeline. Piping is
 * then handled in handle_line. Commands are executed in start_prog, which tests
 * for syntax errors, calls run_child, and waits on the child if necessary.
 *
 */

pid_t child_pid = -127; // process id of the currently running child. -1 if there is no child.

/*  Function name: main
 *  Description: main function of the program. Prompts user for command. If the command is "exit", the program is exited,
 *  otherwise the line is sent to handle_line() which makes
 *  calls to tokenize and attempt to run the appropriate command.
 *  If handle_line() returns non-zero, the user tries again.
 *  Parameter:
 *    argc: argument count.
 *    argv: argument array.
 *  Return:
 *    0 on success.
 */
int main(int argc, char *argv[])
{
  if (signal(SIGTSTP, sigtstp_handler) == SIG_ERR) { // Suspend Key (ctrl + z)
    fprintf(stderr, "Can't handle SIGINT\n");// Not handle ctrl + z
    exit(EXIT_FAILURE);
  }
  
  int status;         // Exit status of child
  int ret;            // Return value of waitpid
  char hostname[128]; // Host names
  if (gethostname (hostname, 128) < 0) // gethostname(char name, int namelen) puts the standard host name for the current machine to name buffer. return 0 if no error occurs.
    strcpy(hostname, "oberlin-cs"); // if gethostname fails, set a host name.
  char line[MAXLEN]; // For receiving from fgets()
  
  for (;;) {
    /* Wait on background processes */
    /* pid_t waitpid(pid_t pid,int * status,int options); */
    ret = waitpid(-1, &status, WNOHANG); // pid = -1: wait for any child process. WNOHANG: if pid(-1) not finished, then return 0; if finished, return the child process' s pid.
    if (ret < 0 && errno != ECHILD) // ECHILD: no child process.
      perror("run_shell: main");
    else if (ret > 0) // ret is the pid of finished child process.
      printf("Child %d exited with status %d\n", ret, status);
    
    /* Display prompt */
    printf("[%s @ %s] ", getenv("USER"), hostname);
    
    /* get next command */
    if (fgets(line, MAXLEN, stdin)) {
      if (!strcmp(line, "\n")) // strcmp: return 0 if str1 == str2.
        continue; // Continue if the input is not a new line.
      if (!strcmp(line, "exit\n"))
        break; // break if input is exit.
      if (!strcmp(line, "about\n")) {
        printf("Name: Xintong Bao Student Id: 1230947\nName: Jingnong Wang Student Id: 1281672\n"); // about command
        continue;
      }
      if (!strcmp(line, "clr\n")) {
        shell_clear(); // clr command
        continue;
      }
      if (strncmp(line, "dir ", 4) == 0) {
        shell_dir(line); // dir command
        continue;
      }
      if (!strcmp(line, "environ\n")) {
        shell_env(); // environ command
        continue;
      }
      if (!strcmp(line, "help\n")) {
        shell_help(); // help command
        continue;
      }
      if(strncmp(line, "cd ", 3) == 0){
        shell_cd(line); // cd command
      }
      else if (handle_line(line)) // Attempt to run the command. Will get 0 on success, 1 on syntax error.
        fprintf(stderr, "run_shell: syntax error\n");
    } else {
      if (errno && (errno != ECHILD)) {
        if (errno == EINTR) // EINTR: Interrupted system call
          continue;
        perror("run_shell: main");
        exit(EXIT_FAILURE);
      } else
      /* EOF */
        break;
    }
  }
  return 0;
}

/* Function name: shell_clear
 * Description: execute shell's clear command.
 */
int shell_clear()
{
  if(system("clear") < 0) {
    printf("Clear failed");
  }
  return 0;
}

/* dir command */
typedef struct dirent DIRENT;
DIR *get_dir(char *filename)
{
  DIR *dir;
  if((dir = opendir(filename)) == NULL)
  {
    printf("Open File %s Error %s\n",filename,strerror(errno));
    exit(1);
  }
  return dir;
}
void print_dir(DIR *dir,DIRENT *dirp)
{
  while((dirp = readdir(dir)) != NULL)
    printf("%s\n",dirp->d_name);
}
int shell_dir(char *arg)
{
  char *argv[CMDNUMBER];
  int argc;
  
  //int parse_args(char *args[], char *arg);
  argc = parse_args(argv, arg);
  argv[argc] = NULL;
  
  DIR *dir;
  DIRENT *dirp = NULL;
  int i = 1;
  if(argc == 1)
  {
    dir = get_dir(getcwd(NULL, 0));
    printf("File:%s\n",getcwd(NULL, 0));
    print_dir(dir, dirp);
    closedir(dir);
    return 0;
  }
  while(argv[i] != NULL)
  {
    dir = get_dir(argv[i]);
    printf("Dir:%s\n",argv[i]);
    print_dir(dir,dirp);
    closedir(dir);
    i++;
  }
  return 0;
}

/* Function name: shell_env
 * Description: execute shell's env command.
 */
int shell_env()
{
  if(system("env") < 0) {
    printf("Get environment parameter failed");
  }
  return 0;
}

/* Function name: shell_help
 * Description: execute shell's help command.
 */
int shell_help()
{
  if(system("help") < 0) {
    printf("Get help manual failed");
  }
  return 0;
}

/* Function name: shell_cd
 * Description: like shell's cd command.
 * Parameters: *arg: arguments from input.
 * Return: 0 on success, -1 on error.
 */
int shell_cd(char *arg)
{
  char *args[CMDNUMBER];
  int argnum;
  
  //int parse_args(char *args[], char *arg);
  argnum = parse_args(args, arg);
  args[argnum] = NULL;
  
  
  char buf[BUFSIZE + 1];
  memset(buf, 0, BUFSIZE + 1);
  
  if(args[1][0] != '/' && args[1][0] != '.'){
    //char *getcwd(char *buf, size_t size);  get current working directory
    if(getcwd(buf, BUFSIZE) == NULL){
      fprintf(stderr, "%s:%d: getcwd failed: %s\n", __FILE__,
              __LINE__, strerror(errno));
      return -1;
    }
    strncat(buf, "/", BUFSIZE - strlen(buf));
  }
  
  //char *strncat(char *dest, const char *src, size_t n);
  strncat(buf, args[1], BUFSIZE - strlen(buf));
  
  //int chdir(const char *path);
  if(chdir(buf) == -1){
    fprintf(stderr, "%s:%d: chdir failed: %s\n", __FILE__,
            __LINE__, strerror(errno));
  }
  return 0;
}

/* Function name: parse arguments
 * Description: count the words in arguments
 * Parameters: *args[]: array that recieves arguments
 *             *arg: arguments
 * Return: count of arguments
 */
int parse_args(char *args[], char *arg)
{
  char **p;
  char *q;
  int ch;
  int count;
  
  p = args;
  q = arg;
  
  
  count = 0;
  while(*q != '\0'){
    
    //int read_char(char *arg);
    while((ch = read_char(q)) == ' '){ /* skip space */
      q++;
    }
    
    *p++ = q++;
    count++;
    
    ch = read_char(q);
    while(ch != ' ' && ch != '\0'){ /* find first space after word */
      q++;
      ch = read_char(q);
    }
    
    if(ch != '\0'){
      *q++ = '\0';
      ch = read_char(q);
    }
  }
  
  return count;
}

/* filter string */
int read_char(char *str)
{
  char filter[] = " \t\n";
  char *p;
  int flag;       /* flag 1 return ' ', 0 return *str */
  
  flag = 0;
  p = filter;
  while(*p != '\0'){
    if(*str == *p){
      flag = 1;
      break;
    }
    p++;
  }
  
  if(flag == 1){
    return ' ';
  }else{
    return *str;
  }
}

/*  Function name: handle_line
 *  Description: Attempt to run a command
 *  Parameters:
 *    line: character pointer to the user's input, terminated with '\n'.
 *  Return:
 *    Returns 0 on success, 1 on syntax error.
 *  Error handling:
 *    Returns 1 on syntax error.
 *    If open() returns -1, an error is printed. If the error is ENOENT (file not
 *    found), then the program is exited. Otherwise 1 is returned (syntax error).
 *    If pipe() returns -1, an error is printed, and the program is exited.
 *    If start_prog returns 1, a syntax error has occurred, and we return 1 to main().
 */
int handle_line(char *line)
{
  /* Remove \n from end of line */
  char *line_ptr = line;
  for ( ; *line_ptr != '\n'; line_ptr++);
  *line_ptr = '\0';
  
  /* Calculate the number of sections separated by pipes in input. */
  int nchunks = 1; // Number of sections
  int i;
  for (i = 0; i < strlen(line); i++) {
    if (line[i] == '|' && (i != 0 && i != strlen(line) - 1))
      nchunks++;
    else if (line[i] == '|' && (i == 0 || i == strlen(line) - 1)) // Syntax error occurs when '|' appears at the start or end of command.
      return 1;
  }
  
  /* Construct array of command structs */
  
  struct command *commands = malloc(nchunks * sizeof(struct command));
  int cur_chunk;
  for (cur_chunk = 0; cur_chunk < nchunks; cur_chunk++) {
    char *chunk;  // Buffer of chars for the current chunk
    if (cur_chunk == 0)
      chunk = strtok(line, "|"); // For the first section, pointer points to start of line. Split line with '|'.
    else
      chunk = strtok(NULL, "|"); // For the subsequent sections, pointer points to the last splitted section. Split remaining part with '|'.
    
    /* Count the number of tokens in chunk */
    char *ptr = chunk;
    char last = ' ';
    int toks = 1;
    while (*ptr) {
      if (isspace(*ptr) && !isspace(last))
        toks++;
      last = *ptr;
      ptr++;
    }
    if (isspace(last))
      toks--;
    
    /* Initialize our command struct for the current chunk */
    commands[cur_chunk].argv = malloc((toks + 1) * sizeof(char *));
    commands[cur_chunk].argc = toks;
    
    /* Tokenize the chunk (split by space) */
    tokenize(chunk, commands[cur_chunk].argv, commands[cur_chunk].argc);
  }
  
  /* If there is no pipe, just run it */
  if (nchunks == 1) {
    /* int start_prog(int pipeno, int numpipes, char *progname, int argc, char *argv[], int fd_in, int fd_out) */
    int ret = start_prog(0, 1, commands[0].argv[0], commands[0].argc, commands[0].argv, 0, 1);
    free_commands(commands, nchunks);
    free(commands);
    return ret;
  }
  
  int p1[2];   // Pipe for parent
  int p2[2];   // Pipe for child
  
  if (pipe(p1)) // return 0 on success, -1 on error. p1[0]: for read; p1[1]: for write.
    perror("run_shell: handle_line");
  
  /* Get started from stdin, stdin --> p1[1] */
  if (start_prog(0, nchunks, commands[0].argv[0], commands[0].argc, commands[0].argv, 0, p1[1])) {
    free_commands(commands, nchunks);
    free(commands);
    return 1;
  }
  close_pipe(p1[1]);
  
  /* multiple pipes */
  
  for (i = 1; i < nchunks - 1; i++) {
    /* Read from parent's pipe, write to child's */
    /* Close the pipe we read from, and the one we write to */
    if (i % 2) { // p1[0] --> p2[1];
      if (pipe(p2))
        perror("run_shell: handle_line");
      if (start_prog(i, nchunks, commands[i].argv[0], commands[i].argc, commands[i].argv, p1[0], p2[1])) {
        free_commands(commands, nchunks);
        free(commands);
        return 1;
      }
      close_pipe(p1[0]);
      close_pipe(p2[1]);
    } else { // p2[0] --> p1[1];
      if (pipe(p1))
        perror("run_shell: handle_line");
      if (start_prog(i, nchunks, commands[i].argv[0], commands[i].argc, commands[i].argv, p2[0], p1[1])) {
        free_commands(commands, nchunks);
        free(commands);
        return 1;
      }
      close_pipe(p2[0]);
      close_pipe(p1[1]);
    }
  }
  
  /* Finish on stdout */
  if (i % 2) { // p1[0] --> stdout
    if (start_prog(i, nchunks, commands[i].argv[0], commands[i].argc, commands[i].argv, p1[0], 1)) {
      free_commands(commands, nchunks);
      free(commands);
      return 1;
    }
    close_pipe(p1[0]);
  } else { // p2[0] --> stdout?
    if (start_prog(i, nchunks, commands[i].argv[0], commands[i].argc, commands[i].argv, p2[0], 1)) {
      free_commands(commands, nchunks);
      free(commands);
      return 1;
    }
    close_pipe(p2[0]);
  }
  
  /* Free every command struct in commands */
  free_commands(commands, nchunks);
  free(commands);
  
  return 0;
}

/* Function name: start_prog
 * Description: Calls run_child, and waits on child if necessary
 * Parameters:
 *   pipeno: pipe number
 *   numpipes: number of pipes
 *   progname, name of program to run
 *   argc: number of arguments
 *   argv, array of arguments. First is program name, last is NULL
 *   fd_in, file descriptor for child's stdin
 *   fd_out, file descriptor for child's stdout
 * Return:
 *   0 on success
 *   1 on syntax error
 * Error handling:
 *   Prints out a message if the progname is not found on the path (run_child() returns -1).
 *   If waitpid returns with an error (-1), a message is printed, and the program is exited.
 */
int start_prog(int pipeno, int numpipes, char *progname, int argc, char *argv[], int fd_in, int fd_out)
{
  /* Check for syntax errors - if argument count were zero */
  if (argc == 0)
    return 1;
  
  /* Find out if there's File I/O */
  int flags;                    // Flags for open()
  int mode = S_IRUSR | S_IWUSR; // Mode for open()
  int i;
  for (i = 0; i < argc; i++) {
    if (!strcmp(argv[i], ">") || !strcmp(argv[i], "<")) { // if the argument is '>' or '<'.
      /* Check syntax error -- redirection where innappropriate */
      if (numpipes > 1) {
        if (!strcmp(argv[i], ">") && pipeno + 1 < numpipes)
          return 1;
        if (!strcmp(argv[i], "<") && pipeno != 0)
          return 1;
      }
      /* Check syntax error -- no file given */
      if (i >= argc)
        return 1;
      
      flags = (!strcmp(argv[i], ">")) ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY; // O_WRONLY: write only; O_CREAT: creat the file; O_TRUNC: clear file; O_RDONLY: read only.
      int fd_tmp = open(argv[i+1], flags, mode); // int open(const char *pathname, int flags, mode_t mode); return fd on success, -1 on error.
      /* process error from open() */
      if (fd_tmp < 0) {
        perror("run_shell: start_prog");
        if (errno != ENOENT) /* Consider ENOENT (file not found) a syntax error */
          exit(EXIT_FAILURE);
        return 1;
      }
      fd_in = (!strcmp(argv[i], "<")) ? fd_tmp : fd_in; // choose fd_in and fd_out by the direction.
      fd_out = (!strcmp(argv[i], ">")) ? fd_tmp : fd_out;
      argv[i] = NULL;
    }
  }
  
  /* Run in background if requested */
  if (!strcmp(argv[argc-1], "&")) {
    argv[argc-1] = NULL;
    if ((child_pid = run_child(progname, argv, fd_in, fd_out, 2)) < 0) {
      /* error */
      perror("Command not found on the path");
      child_pid = -127;
    }
  } else {
    child_pid = run_child(progname, argv, fd_in, fd_out, 2);
    if (child_pid < 0) {
      /* run_child returned error */
      perror("Command not found on the path");
    } else {
      int status;
      if (waitpid(child_pid, &status, 0) < 0) {
        /* Error -- check if control-c was sent */
        if (errno == EINTR) {
          printf("Exiting process %d\n", child_pid);
          return 0;
        }
        perror("run_shell: handle_line");
        exit(EXIT_FAILURE);
      }
      child_pid = -127;
    }
  }
  
  return 0;
}

/* Function name: close_pipe
 * Description: Closes a file descriptor.
 * Parameter:
 *   fd: file descriptor to be closed.
 * Error Handling:
 *   If EINTR is returned by close, tries until success or worse error.
 *   If EBADF or EIO are returned by close, perror is called and the program is terminated.
 */
void close_pipe(int fd)
{
  int ret;
  while ((ret = close(fd))) { // close(): return 0 on success, -1 on error.
    if (ret == EBADF || ret == EIO) { // EBADF: invalid file or file has already closed. EIO: I/O error.
      perror("run_shell: close_pipe");
      exit(EXIT_FAILURE);
    }
  }
}

/* Function name: free_commands
 * Description: Frees all command structs in an array.
 * Parameter:
 *   *cmds: Command struct array to free.
 *   len: Length of the array.
 */
void free_commands(struct command *cmds, int len)
{
  int i;
  for (i = 0; i < len; i++) {
    free(cmds[i].argv);
  }
}

/* Function name: sigtstp_handler
 * Description: Signal handler for SIGTSTP
 * Parameter:
 *   signo: signal to be processed.
 */
static void sigtstp_handler(int signo)
{
  if (child_pid > 0) {
    kill(child_pid, signo);
    child_pid = -127;
  }
}
