#ifndef SHELL_H
#define SHELL_H

#include "fileSys.h"

#define MAXCMD 1024
#define MAXARGS 16
#define AVAILABLECMDS 17

///Structures:
// commandStruct stores the information necessary for the shell to execute commands
typedef struct {
    int argNumber;
    char command[MAXCMD];
    char arguments[MAXARGS][MAXCMD];
} commandStruct;


/// Session
void initialize();

/// Command extraction:
// obtains user input and returns it in the form of commandStruct
commandStruct getCommand();
// divides the user input into a command and its arguments
commandStruct parse(char*);

///Command execution
// dispatcher function that executes the command requested by the user
void execute(commandStruct);
// maps the command string to a corresponding index for execution
int getCmdIdx(char*);

#endif // SHELL_H
