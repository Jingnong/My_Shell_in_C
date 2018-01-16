/*  File name: myshell.h
 *  Project name: project1
 *  Author: Xintong Bao, Jingnong Wang
 *  Date: 04/09/2017
 */
#ifndef myshell_h
#define myshell_h

/* Represents a command (that may be part of a pipe sequence) */
struct command {
    char **argv;
    int argc;
};

/* Get rid of compile warnings */
int gethostname (char *name, size_t len);
int kill (pid_t pid, int signo);
int read_char(char *str);  
int parse_args(char *args[], char *arg);  
int shell_cd(char *args);
int shell_clear();
int shell_dir(char *arg);
int shell_env();
int shell_help();
int handle_line (char *line);
int start_prog (int pipeno, int numpipes, char *progname, int argc, char *argv[], int fd_in, int fd_out );
void close_pipe (int fd);
void free_commands (struct command *cmds, int len);
static void sigtstp_handler (int signo);

#endif /* myshell_h */
