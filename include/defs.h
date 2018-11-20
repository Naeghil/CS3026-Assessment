#ifndef DEFS_H_INCLUDED
#define DEFS_H_INCLUDED

///this file includes basic macro definitions
///as well as low level typedefs

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
#define DIRENTRYCOUNT ((BLOCKSIZE - (1*sizeof(int)) ) / sizeof(direntry_t))
#define MAXNAME       128
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
/*
typedef struct direntry {
   Byte        isdir ; //does the entry represent a directory or a file?
   Byte        unused ; //is the entry used?
   time_t      modtime ; //last modified
   int         filelength ; // This is wasted if the entry represents a subdirectory
   fatentry_t  firstblock ; // the first block of the file, or the block of the sub-dirblock
   char   name [MAXNAME] ; // the name of the entry; could actually be used instead of "unused"
} direntry_t ; //max subdir/files is 7 with 128char names .-. */
//man this'll be super hard to maintain fk
typedef struct dirEntry {
    time_t          modtime; //time last modified
    fatentry_t      firstblock; // first block of a file; dirblock of the entry for subdirectories?
    signed char     nameLength; //supports names up to 127 characters, extension included; if -1, the entry is unused
    signed char     childrenNo; //supports a maximum of 127 children; -1 if it's a file
    char            name[]; // name of the entry
} direntry_t;

///a directory block is an array of directory entries
/*
typedef struct dirblock {
   int isdir ;
   direntry_t entrylist [ DIRENTRYCOUNT ] ; //a directory can only have DIRENTRYCOUNT between subdirs and files?
} dirblock_t ; */
//fffffffffkkkk; it needs compaction once in a while
typedef struct dirblock {
    short       nextId; // the id of the first entry
    char        entryCount;
    char        isFull; // are all the entries used? this will be needed to "compact" the filesys once in a while
    direntry_t  entries[];
} dirblock_t;

/// a block can be either a directory block, a FAT block or actual data
typedef union block {
   datablock_t data ;
   dirblock_t  dir  ;
   fatblock_t  fat  ;
} diskblock_t ;

/// created when a file is opened on the disk
typedef struct filedescriptor {
   char        mode[3] ;
   fatentry_t  firstBlock ;
   fatentry_t  blockNo ;           // block no
   int         pos     ;           // byte within a block
   diskblock_t buffer  ;
} MyFILE;

///used to provide 2-level indexing for the directory hierarchy
typedef struct dynamicDirTable {
    int arraySize;
    int realBlocksNo;
    short* entryNumbers;
} directoryTable;
//create a new directory table
/*
void create(directoryTable *t) {
    t->arraySize = 10;
    t->entryNumbers = malloc(sizeof(short)*t->arraySize);
    t->realBlocksNo = 1;
    t->entryNumbers[0] = 0;
}; */
//insert a new block with idx number of entries
/*
void ins(directoryTable *t, short idx) {
    if(t->realBlocksNo==t->arraySize) {
        short temp[t->arraySize];
        for(int i=0; i<t->arraySize; i++) temp[i] = t->entryNumbers[i];
        t->arraySize = t->arraySize*2;
        t->entryNumbers = malloc(sizeof(short)*t->arraySize);
        for(int i=0; i<t->realBlocksNo; i++) t->entryNumbers[i] = temp[i];
    };
    t->entryNumbers[t->realBlocksNo] = idx;
    t->realBlocksNo++;
};
//add an entry to the block blockNo
void add(directoryTable *t, int blockNo) { if(blockNo<t->realBlocksNo) t->entryNumbers[blockNo]++; };
//remove an entry to the block blockNo
void rem(directoryTable *t, int blockNo) { if(blockNo<t->realBlocksNo) t->entryNumbers[blockNo]--; };
int getVirtualIdx(directoryTable *t, short entryIdx) {
    int idx=0;
    while(entryIdx>t->entryNumbers[idx]) {
        entryIdx-=t->entryNumbers[idx];
        idx++;
    };
    return idx;
}; */




#endif // DEFS_H_INCLUDED
