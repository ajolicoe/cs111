/*
 * Authors:
 *    Alex Jolicoeur <ajolicoe@ucsc.edu>
 *    Ruben Lopez    <rklopez@ucsc.edu>
 *    Alex Lowe      <aolowe@ucsc.edu>
 *    Jesse Kunz     <jckunz@ucsc.edu>
 * Purpose:
 * This file implements a basic shell for unix.
 * It is written for assignment 1 of cmps111
 * Assumptions:
 * 1. The program will be running on a unix based platform
 * 2. The input will be via keyboard, and the output will be on the terminal
 * 3. The user has some basic knowledge of unix commands
 *
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

extern char **get_line();

void execute(char **, int, char **);
char **parse(char **, char **, int *);

#define DEFAULT_MODE         0
#define BACKGROUND_MODE      1
#define OUTPUT_REDIRECT_MODE 2
#define INPUT_REDIRECT_MODE  3
#define PIPELINE_MODE        4

int main(int argc, char *argv[]) {
  int mode = DEFAULT_MODE;
  int commandArgc;
  char *supp = NULL;// file supplement
  char **args, **command;// tokenized command from flex

  while (1) {
    mode = DEFAULT_MODE;
    printf("myShell>"); //display prompt
    args = get_line();
    if (args[0] 
    && strcmp(args[0], "<") != 0
    && strcmp(args[0], ">") != 0
    && strcmp(args[0], "&") != 0
    && strcmp(args[0], "|") != 0) {
      // check for exit
      if (strcmp(args[0], "exit") == 0) exit(0);
      // parse args, and set mode
      command = parse(args, &supp, &mode);
      if (strcmp(args[0], "cd") == 0) chdir(args[1]);
      else execute(command, mode, &supp); //execute command
    }
  }
  return 0;
}

/*
 * parse(char **, char *[], char **, int *)
 * parses input command, sets the desired mode and supplement pointer.
 
 * @param input   char**  - tokenized input command
 * @param suppPtr char**  - file supplement pointer to update
 * @param modePtr int*    - operation mode to update
 * @return commandArgc int - number of command arguments
 */
char **parse(char **input, char **suppPtr, int *modePtr) {
  int i, commandArgc = 0;
  int terminate = 0;
  *suppPtr = NULL;
  for (i = 0; input[i] != NULL && !terminate; i++) {
    commandArgc++;
    if (strcmp(input[i], "&") == 0) {
      // background mode, no file supplement, no terminate
      *modePtr = BACKGROUND_MODE;
      input[i] = '\0';
    } else if (strcmp(input[i], ">") == 0) {
      // output redirect, terminate
      *modePtr = OUTPUT_REDIRECT_MODE;
      // file supplement is token after '>'
      *suppPtr = input[i+1];
      input[i] = '\0';
      terminate = 1;
    } else if (strcmp(input[i], "<") == 0) {
      // input redirect, terminate
      *modePtr = INPUT_REDIRECT_MODE;
      // file supplemenet is token after '<'
      *suppPtr = input[i+1];
      input[i] = '\0';
      terminate = 1;
    } else if (strcmp(input[i], "|") == 0) {
      // pipeline, terminate
      *modePtr = PIPELINE_MODE;
      // file supplement is token after '|'
      *suppPtr = input[i+1];
      input[i] = '\0';
      terminate = 1;
    }
  }
  return input;
}

/*
 * execute(char**, int, char**)
 * executes the input command using execvp 
 *
 * @param command char** - input command to execute
 * @param mode    int    - mode of command
 * @param suppPtr char** - file supplement pointer
 */
void execute(char **command, int mode, char **suppPtr) {
  pid_t pid1, pid2;
  FILE *file;
  // vars for pipeline
  char *supp2 = NULL;
  char **command2;
  int commandArgc2;
  int mode2 = DEFAULT_MODE;
  int pid1_status, pid2_status;
  int pipeline[2];
  if (mode == PIPELINE_MODE) {
    int pipelining = pipe(pipeline);
    if (pipelining) {
      fprintf(stderr,"Pipe failed!\n");
      exit(-1);
    }
    // pipeline mode, parse supplement pointer as second command
    command2 = parse (suppPtr, &supp2, &mode2);
  }
  if (mode == BACKGROUND_MODE) {
    // raise signal when background child is terminated
    // (remove zombie process)
    signal(SIGCHLD, SIG_IGN);
  }
  // fork child process
  pid1 = fork();
  if (pid1 < 0) {
    printf("Error: pid1 < 0\n");
    exit(1);
  } else if (pid1 == 0) {
    // is pid1 child process
    switch (mode) {
      case OUTPUT_REDIRECT_MODE:
        // open file supplement for reading/writing from stdout stream
        file = freopen(*suppPtr, "w+", stdout);
        // set file descriptor of file to stdout, and close stdout
        dup2(fileno(file), 1);
        break;
      case INPUT_REDIRECT_MODE:
        // open file supplement for reading from stdin stream
        file = freopen(*suppPtr, "r", stdin);
        // set file descriptor of file to stdin, and close stdin
        dup2(fileno(file), 0);
        break;
      case PIPELINE_MODE:
        // close first pipeline command
        close(pipeline[0]);
        // set file descriptor of second command to stdout, and close stdout
        dup2(pipeline[1], 1);
        // close second pipeline command
        close(pipeline[1]);
        break;
    }
    // execute first child command
    execvp(*command, command);
  } else { // pid1 > 0
    // pid1 is parent process
    if (mode == BACKGROUND_MODE);
    else if (mode == PIPELINE_MODE) {
      // is parent, so wait for child pid1
      waitpid(pid1, &pid1_status, 0);
      // fork child process
      pid2 = fork();
      if (pid2 < 0) {
        printf("Error: pid2 < 0\n");
        exit(1);
      } else if (pid2 == 0) {
        // pid2 is child process, close second pipeline command
        close(pipeline[1]);
        // set file descriptor of first command to stdin, and close stdin
        dup2(pipeline[0], 0);
        // close first pipeline command
        close(pipeline[0]);
        // execute second child command
        execvp(*command2, command2);
      } else {
        //close pipeline
        close(pipeline[0]);
        close(pipeline[1]);
      }
      // pid2 is parent process, wait for pid1
    } else waitpid(pid1, &pid1_status, 0);
  }
}
