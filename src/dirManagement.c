#include "dirManagement.h"

extern fileSys* directoryHierarchy;
extern fileSys* workingDir;
extern MyFILE* currentlyOpen;

///TODO: check if this stuff is needed
//maintain the number of entries per block to speed up entry retrieval
//This works like a two-level indexing: when an entry is needed, iterate through this array
//and subtract the values each time
//an entry has a minimum sie of 14b and a maximum size of 394b; the medium is of
extern directoryTable  secondLvlDirIdx;



direntry_t* initDirEntry(time_t mTime, short fBlock, signed char nLen, signed char cNo, char* name) {
    direntry_t *newEntry;
    newEntry = malloc(sizeof(time_t)+sizeof(short)*2+2+nLen);
    newEntry->childrenNo = cNo;
    newEntry->firstblock = fBlock;
    newEntry->modtime = mTime;
    newEntry->nameLength = nLen;
    memset(newEntry->name, '\0', nLen);
    strcpy(newEntry->name, name);
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
        //time_t+short = 6
        memmove(&entryNameSize, &block->data[offset+6], sizeof(char));
        //time_t+short+2*char = 8
        offset += 8+entryNameSize;
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

fileSys* makeDirTree(direntry_t all[], int idx, int parentidx) {
    //make an array of nodes using makeNode
    //use this function to connect them together in a tree-way
}



fileSys* makeNode(char* name, time_t modTime, fatentry_t fBlock, short childrenNo) {
    fileSys *toRet = malloc(sizeof(fileSys));
    toRet->name = malloc(sizeof(name));
    strcpy(toRet->name, name);
    toRet->modTime = modTime;
    toRet->firstBlock = fBlock;
    toRet->children = malloc(sizeof(fileSys*)*childrenNo);
    for(int i=0; i<sizeof(toRet->children); i++) toRet->children[i] = NULL;

    return toRet;
}