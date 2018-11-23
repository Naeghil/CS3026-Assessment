#ifndef SHELL_H
#define SHELL_H

#include "fileSys.h"

#define MAXCMD 256
#define MAXARGS 16
#define AVAILABLECMDS 17

///Structures:
// commandStruct stores the information necessary for the shell to execute commands
typedef struct {
    int argNumber;
    char* command;
    char* arguments[MAXARGS];
} commandStruct;


/// Session
void initialize();

/// Command extraction:
// obtains user input and returns it in the form of commandStruct
commandStruct getCommand();
// divides the user input into a command and its arguments
commandStruct parse(char*);
// a small utility for spaced arguments
void spacedArg(commandStruct *, int);
// initializes a commandStruct
void initCommand(commandStruct*);

///Command execution
// dispatcher function that executes the command requested by the user
void execute(commandStruct);
// maps the command string to a corresponding index for execution
int getCmdIdx(char*);

///Command Functions:
// prints to screen the description for the specified command (all if no argument provided);
void printManual(int, char**);
    ///Session commands
//exits the shell but tries and save the disk to file; -f doesn't save the disk
void quit(int, char**);
//saves the disk to file
void save(int, char**);
//loads a disk from file
void load(int, char**);
    ///Directory commands
//changes the working directory to the specified directory; to root if no argument provided
void cd(int, char**);
//lists the contents of the directory specified; the current if no arguments provided
void ls(int, char**);
//creates a new directory (together with specified subdirectories); can accept absolute path
void mkDir(int, char**);
//removes a file or directory (-f removes all subdirectories too)
void rm(int, char**);
//moves a file/directory to a specified directory; also used to rename stuff
void mv(int, char**);
//copies a file/directory to a specified directory
void cp(int, char**);
    ///File editing commands
//opens an existing file or creates a new one if it doesn't exist
void myOpen(int, char**);
//closes the open file and saves its contents to disk (args?)
void closef(int, char**);
//saves the contents of an open file to the virtual disk
void savef(int, char**);
// prints to screen the contents of an open file, or opens it first if it's not open
void myRead(int, char**);
// reads the last line of an open file; it opens it first if it's not open
void readl(int, char**);
// appends a line to an open file; it opens it first if it's not open
void writel(int, char**);
// removes the last line of an open file; doesn't work if it's not open
void removel(int, char**);

#endif // SHELL_H
