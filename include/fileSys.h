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
//closes the file
void myfclose ();
//returns the next byte of the open file, or EOF (EOF == -1)
int myfgetc ();
//Writes a byte to the file. If buffer is full, write to disk
void myfputc ( int );
//Writes the remaining contents of the open file to disk
bool saveFile();
//Prints all contents of a closed file
void readFile( pathStruct );
//Prints all contents of the currently open file
void printFile();
//Prints the last n lines of the currently open file
void printLine( int );
//Appends the string to the currently open file
void appendLine();
//eliminates the last line of the currently open file
void deleteLine();


///Directory Management Functions
//Creates a new directory as specified, recursively.
void mymkdir ( pathStruct );
//Removes an existing file
void myremove ( pathStruct );
//Removes an existing directory and all its contents recursively, or file
void myrmdir ( pathStruct );
//Changes into an existing directory
void mychdir ( pathStruct );
//Returns an array of strings listing the contents of an existing directory
char ** mylistdir ( pathStruct );
//moves source to destination
void myMvDir( pathStruct, pathStruct );
//copies source to destination
void myCpDir( pathStruct, pathStruct );
//Computes a pathStruct out of a path string and checks its validity
pathStruct parsePath(const char*);
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
bool isDiskFile(FILE *);
void loadDiskFromFile(FILE*);
void initStructs();



#endif // FILESYS_H
