#ifndef DIRMANAGEMENT_H
#define DIRMANAGEMENT_H

#include "defs.h"



void openFile(const char*, MyFILE*);

pathStruct getPath(dirNode*);
dirNode* makeDirTree(direntry_t*, int);
///entry manipulation
direntry_t initDirEntry(time_t, short, char, char*);
void insertDirEntry(diskblock_t*, direntry_t*);
int getEntryOffset(diskblock_t*, int);
direntry_t* getEntry(diskblock_t*, int);
///dirNode manipulation
dirNode* makeNode(direntry_t);
dirNode* createNode(char*, fatentry_t, int, dirNode*);
void destroyDir(dirNode*);
void removeDir(dirNode* , dirNode* );
void appendDir(dirNode* , dirNode* );


#endif // DIRMANAGEMENT_H
