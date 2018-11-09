#include "dirManagement.h"

extern pathStruct root;
extern fileSys directoryHierarchy;
extern fileSys* workingDir;
extern MyFILE* currentlyOpen;

///TODO: check if this stuff is needed
//maintain the number of entries per block to speed up entry retrieval
//This works like a two-level indexing: when an entry is needed, iterate through this array
//and subtract the values each time
//an entry has a minimum sie of 14b and a maximum size of 394b; the medium is of
extern directoryTable  secondLvlDirIdx;



direntry_t* initDirEntry(time_t mTime, short parent, short fBlock, signed char nLen, signed char cNo, char* name) {
    direntry_t *newEntry;
    newEntry = malloc(sizeof(time_t)+sizeof(short)*2+2+nLen);
    newEntry->childrenNo = cNo;
    newEntry->firstblock = fBlock;
    newEntry->modtime = mTime;
    newEntry->nameLength = nLen;
    memset(newEntry->name, '\0', nLen);
    strcpy(newEntry->name, name);
    newEntry->parent = parent;
    return newEntry;
}

diskblock_t initDirBlock(short nextIdx) {
    diskblock_t newDir;
    for(int i=0; i<BLOCKSIZE; i++) newDir.data[i] = '\0';
    newDir.dir.entryCount = 0;
    newDir.dir.isFull = false;
    newDir.dir.nextId = nextIdx;
    return newDir;
}

void insertDirEntry(diskblock_t* block, direntry_t* entry) {
    int offset = getEntryOffset(block, block->dir.entryCount);
    memmove(&block->data[offset], entry, sizeof(*entry)+entry->nameLength);
    block->dir.entryCount++;
}

int getEntryOffset(diskblock_t* block, int idx) {
    int offset = 5;
    for(int i=0; i<idx; i++) {
        char entryNameSize;
        //time_t = 4B, short = 2B, char = 1B
        //time_t+2*short = 8
        memmove(&entryNameSize, &block->data[offset+8], sizeof(char));
        //time_t+2*short+2*char = 10
        offset += 10+entryNameSize;
    }
    return offset;
}

direntry_t* getEntry(diskblock_t* block, int idx) {
    int offset = getEntryOffset(block, idx-block->dir.nextId);
    int size = getEntryOffset(block, idx-block->dir.nextId+1) - offset;
    direntry_t* toReturn = malloc(size);
    memmove(toReturn, &block->data[offset], size);
    return toReturn;
}

