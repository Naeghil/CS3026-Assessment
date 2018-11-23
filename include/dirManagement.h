#ifndef DIRMANAGEMENT_H
#define DIRMANAGEMENT_H

#include "defs.h"



//Initializes the runtime structure of the filesystem
dirNode* makeDirTree(direntry_t*, int);
//Entry manipulation
direntry_t initDirEntry(time_t, short, char, char*);

///dirNode manipulation
void openFile(char*, MyFILE*);
//Makes a node out of an entry, used to initialize the structures
dirNode* makeNode(direntry_t);
//Effectively creates a new node
dirNode* createNode(char*, fatentry_t, int, dirNode*);
//Used to manipulate the children of a directory
void removeDir(dirNode* , dirNode* );
void appendDir(dirNode* , dirNode* );


#endif // DIRMANAGEMENT_H
