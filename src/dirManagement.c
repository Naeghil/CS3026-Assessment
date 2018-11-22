#include "../include/dirManagement.h"

extern dirNode* directoryHierarchy;
extern dirNode* workingDir;
extern MyFILE* currentlyOpen;

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

void insertDirEntry(diskblock_t* block, direntry_t* entry) {
    int offset = getEntryOffset(block, block->dir.entryCount);
    memmove(&block->data[offset], entry, sizeof(*entry)+entry->nameLength);
    block->dir.entryCount++;
}

int getEntryOffset(diskblock_t* block, int idx) {
    ///TODO: check this
    int offset = 1;
    for(int i=0; i<idx; i++) {
        char entryNameSize;
        //time_t = 4B, short = 2B, char = 1B
        //time_t+short = 6
        memmove(&entryNameSize, &block->data[offset+6], 1);
        //time_t+short+2*char = 8
        offset += 8+entryNameSize;
    }
    return offset;
}

direntry_t* getEntry(diskblock_t* block, int idx) {
    int offset = getEntryOffset(block, idx);
    int size = getEntryOffset(block, idx+1) - offset;
    direntry_t* toReturn = malloc(size);
    memmove(toReturn, &block->data[offset], size);
    return toReturn;
}

dirNode* makeDirTree(direntry_t all[], int cardinality) {
    dirNode** allNodes = malloc(sizeof(dirNode*)*cardinality);
    for(int i=0; i<cardinality; i++) allNodes[i] = makeNode(all[i]);
    int childIdx = 1;
    for(int nodeIdx = 0; nodeIdx<cardinality; nodeIdx++) {
        for(int i = 0; i<allNodes[nodeIdx]->childrenNo; i++) {
            allNodes[nodeIdx]->children[i] = allNodes[childIdx];
            allNodes[childIdx]->parent = allNodes[nodeIdx];
            childIdx++;
        }
    }
    dirNode* root = allNodes[0];
    free(allNodes);
    return root;
}

dirNode* makeNode(direntry_t entry) {
    dirNode *toRet = malloc(sizeof(dirNode));
    toRet->name = malloc(entry.nameLength);
    strcpy(toRet->name, entry.name);
    toRet->modTime = entry.modtime;
    toRet->firstBlock = entry.firstblock;
    toRet->childrenNo = entry.childrenNo;
    toRet->children = malloc(sizeof(dirNode*)*entry.childrenNo);
    for(int i=0; i<toRet->childrenNo; i++) toRet->children[i] = NULL;

    return toRet;
}

dirNode* createNode(char* name, fatentry_t fBlock, int cNo, dirNode* parent) {
    dirNode* toRet = malloc(sizeof(dirNode));
    toRet->name = malloc(sizeof(name));
    strcpy(toRet->name, name);
    time(&toRet->modTime);
    toRet->firstBlock = fBlock;
    toRet->childrenNo = cNo;
    toRet->children = malloc(sizeof(dirNode*)*cNo);
    toRet->parent = parent;
    return toRet;
}

void destroyDir(dirNode* dir) {
    free(dir->name);
    free(dir->children);
    free(dir);
}

void removeDir(dirNode* parent, dirNode* child){
    dirNode** buff = malloc(sizeof(dirNode*)*(parent->childrenNo-1));
    int idx = 0;
    for(;parent->children[idx]!=child; idx++) buff[idx] = parent->children[idx];
    for(int i=idx; i<parent->childrenNo-2; i++) buff[i] = parent->children[i+1];
    free(parent->children);
    parent->children = buff;
}

void appendDir(dirNode* parent, dirNode* child) {
    child->parent = parent;
    dirNode** buff = malloc(sizeof(dirNode*)*(parent->childrenNo+1));
    for(int i=0; i<parent->childrenNo; i++) buff[i] = parent->children[i];
    buff[parent->childrenNo] = child;
    free(parent->children);
    parent->children = buff;
}

