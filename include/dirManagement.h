#ifndef DIRMANAGEMENT_H
#define DIRMANAGEMENT_H

#include "defs.h"

typedef struct {
    bool isFile;
    bool isEmpty;
    bool isAbsolute;
    //NULL terminating array of existing path "components"
    char** existing;
    //NULL terminating array of non-existing path "components"; determines path validity
    char** nonExisting;
} pathStruct;

typedef struct fileSystemNode {
    char* name; //name of the file or directory
    short childrenNo;
    struct fileSystemNode* parent;
    struct fileSystemNode* children[];
} fileSys;


void openFile(const char*, MyFILE*);
void newFile(const char*, MyFILE*);

pathStruct getPath(fileSys*);

diskblock_t initDirBlock(short);
direntry_t* initDirEntry(time_t, short, fatentry_t, signed char, signed char, char*);
void insertDirEntry(diskblock_t*, direntry_t*);
int getEntryOffset(diskblock_t*, int);
direntry_t* getEntry(diskblock_t*, int);
fileSys* makeNode(direntry_t* );
bool isFile(fileSys* node) { return node->childrenNo==-1; };



#endif // DIRMANAGEMENT_H
