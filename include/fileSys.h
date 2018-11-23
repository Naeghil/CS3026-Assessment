#ifndef FILESYS_H
#define FILESYS_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fat.h"
#include "dirManagement.h"
#include "virtualDisk.h"

///File Management Functions
//Opens a file, manages a buffer[BLOCKSIZE]
//mode: "r" readonly; "w" read/write/append
MyFILE * myfopen ( pathStruct , const char * ) ;
//Creates a new dirNode for a file, with parent path.dir
//also clears the block and adds an EOF for the data
dirNode* newFile(char*, dirNode*);
//closes the file
void myfclose ();
//returns the next byte of the open file, or EOF (EOF == -1)
//if buffer is full, go to the next
char myfgetc ();
//Writes a byte to the file. If buffer is full, write to disk
void myfputc ( char );
//Removes the last char and handles blocks
void myfremc();
//Writes the remaining contents of the open file to disk
void saveFile();
//Prints all contents of a closed file
void readFile( pathStruct );
//Prints all contents of the currently open file
void printFile();
//Prints the last n lines of the currently open file
void printLine( int );
//Appends the string to the currently open file
void appendLine( char* );
//eliminates the last line of the currently open file
void deleteLine();

///Directory Management Functions
//Creates a new directory as specified, recursively.
void mymkdir ( pathStruct );
//Removes an existing file
void myremove ( pathStruct );
//Removes an existing directory and all its contents recursively, or file
void myrmdir ( pathStruct );
void recurRmDir(dirNode*);
//Changes into an existing directory
void mychdir ( pathStruct );
//Returns a string of \t separated contents of a directory
//for further use, it can be easily separated with strtok(content, "\t");
char * mylistpath ( pathStruct );
char * mylistdir ( dirNode* );
//moves source to destination
void myMvDir( pathStruct, pathStruct );
//copies source to destination
void myCpDir( pathStruct, pathStruct );
void recurCp(dirNode*, dirNode*);
dirNode* cpyFile(char*, dirNode*);
//Computes a pathStruct out of a path string and checks its validity
pathStruct parsePath(char*);
//returns the full path of the working directory, with ending colon
char* workingDirPath();

///Functions for virtualDisk
void format();
//writes fat to virtual disk
void writeFat();
//writes the directory hierarchy to the virtual disk
void writeDirectory();
//Writes FAT and directory to disk; this shouldn't take care of eventually open files;
void saveVDisk();
//Initializes runtime structures
void initStructs();



#endif // FILESYS_H
