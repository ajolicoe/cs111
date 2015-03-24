Purpose:

This document specifies the design of a basic shell program for unix.

Assumptions:

1. The user is using a unix based platform
2. the input will be through the keyboard, the output will be through the terminal.
3. The user has some knowledge unix commands

Data:

Commands - The commands are typed into the terminals and are parsed by Flex for the our program to read.

Operations:

1. Parse(char **input, char **supplement, int *mode)
Description: a function that parses user commands, setting context and supplements, for all shell commands

    Input: 
        * a tokenized input command
        * File supplement pointer to set
        * operation mode to set

    Output: the number of tokens in the command

    Result: The user's command has been parsed, the mode pointer and the file supplement
        pointer have both been set.

2. Execute:(char **command, int mode, char **supplement)
Description: Executes a parsed command based on it's mode and file supplement

    Input: the input command to execute
        * the mode of command
        * a file supplement pointer
    
    Output: none

    Result: Decides which operation the command fits under, 'BACKGROUND', 'OUTPUT_REDIRECT', or 'INPUT_REDIRECT'
        and calls execvp after redirecting the appropriate file descriptors.

Algorithms:

Basic shell Algorithm:

1. set mode to default
2. while not exiting    
    1. print out prompt
    2. Wait for user input
    3. check for 'exit', if present we exit(0)
    4. parse the input command
    5. check for 'cd' command
    6. if command was neither 'exit' nor 'cd' then we call the exectute function on command

Parse algorithm:

1. for each token of the input command array:
    1. if token is '&' set mode pointer to background mode, do not terminate.
    2. else if token is '>':
        1. set the mode pointer to output redirect mode.
        2. set the file supplement pointer to the token after the '&'.
        3. terminate parse.
    3. else if token is '<':
        1. set the mode pointer to input redirect mode.
        2. set the file supplement pointer to the token after the '<'.
        3. terminate parse.
    4. else if token is '|':
        1. set the mode pointer to pipeline mode.
        2. set the command supplement to the token after the '|'.
        3. terminate parse.
2. return input

Execute Algorithm:

1. check the mode parameter
2. if background mode: 
    1. send signal when terminated so no zombie processes are created
3. fork() the process.
4. if fork() returns -1 exit(-1)
5. otherwise if fork returns 0 its the child; check the child's mode
    1. if output redirect mode:
        1. open the supplement pointer as a file for reading/writing
        2. set the file descriptor to copy from stdout (dup2)
    2. if Input redirect mode:
        1. open the supplement pointer as a file for reading
        2. set the file descriptor to copy from stdin
    3. call execvp on the command
6. otherwise if fork() > 0 it's the parent process
    1. if background mode:
        do nothing, we handled to termination with the signal before.
    2. else wait for child to finish (waitpid)
