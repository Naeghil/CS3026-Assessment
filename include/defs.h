#ifndef DEFS_H_INCLUDED
#define DEFS_H_INCLUDED

///this file includes basic macro definitions
///as well as low level typedefs
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAXBLOCKS     1024
#define BLOCKSIZE     1024
#define FATENTRYCOUNT (BLOCKSIZE / sizeof(fatentry_t))
#define FATBLOCKSNO  (MAXBLOCKS / FATENTRYCOUNT )
#define DIRENTRYCOUNT 8
#define MAXNAME       121
#define MAXPATHLENGTH 1024

#define UNUSED        -1
#define ENDOFCHAIN     0

#ifndef EOF
#define EOF           -1
#endif

typedef unsigned char Byte ;
typedef short fatentry_t ;

/// a FAT block is a list of 16-bit entries that form a chain of disk addresses
typedef fatentry_t fatblock_t [ FATENTRYCOUNT ] ;

/// a data block holds the actual data of a filelength, it is an array of 8-bit (byte) elements
typedef Byte datablock_t [ BLOCKSIZE ] ;

/// create a type direntry_t
typedef struct dirEntry {
    time_t      modtime; //time 0 if unused
    fatentry_t  firstblock; // first block of a file; dirblock of the entry for subdirectories?
    char        childrenNo; //supports a maximum of 127 children; -1 if it's a file
    char        name[MAXNAME]; // name of the entry
} direntry_t; //size of a single entry is 128

///a directory block is an array of directory entries
typedef direntry_t dirblock_t [DIRENTRYCOUNT];

/// a block can be either a directory block, a FAT block or actual data
typedef union block {
   datablock_t data ;
   dirblock_t  dir  ;
   fatblock_t  fat  ;
} diskblock_t ;

/// created when a file is opened on the disk
typedef struct filedescriptor {
   char        mode[2] ;
   fatentry_t  firstBlock ;
   fatentry_t  blockNo ;           // block no
   int         pos     ;           // byte within a block
   diskblock_t buffer  ;
} MyFILE;

typedef struct fileSystemNode {
    char* name; //name of the file or directory
    time_t modTime;
    fatentry_t firstBlock;
    struct fileSystemNode* parent;
    int childrenNo;
    struct fileSystemNode** children;
} dirNode;

typedef struct {
    bool isFile;
    bool isValid;
    //The directory the path points at
    dirNode* dir;
    //NULL terminating array of non-existing path "components"; determines path validity
    char* nonExisting[16];
} pathStruct;


#endif // DEFS_H_INCLUDED
