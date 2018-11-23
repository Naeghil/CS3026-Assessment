#include "../include/dirManagement.h"

extern dirNode* directoryHierarchy;
extern dirNode* workingDir;
extern MyFILE* currentlyOpen;

direntry_t* initDirEntry(time_t mTime, short fBlock, char cNo, char* name) {
    direntry_t *newEntry = malloc(sizeof(direntry_t));
    newEntry->childrenNo = cNo;
    newEntry->firstblock = fBlock;
    newEntry->modtime = mTime;
    memset(newEntry->name, '\0', MAXNAME);
    strcpy(newEntry->name, name);
    return newEntry;
}

dirNode* makeDirTree(direntry_t all[], int cardinality) {
    dirNode** allNodes = malloc(sizeof(dirNode*)*cardinality);
    for(int i=0; i<cardinality; i++) allNodes[i] = makeNode(all[i]);
    allNodes[0]->parent = NULL;
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
    toRet->name = malloc(sizeof(entry.name));
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
    int idx;
    for(idx = 0; parent->children[idx]!=child; idx++) buff[idx] = parent->children[idx];
    for(int i=idx; i<parent->childrenNo-1; i++) buff[i] = parent->children[i+1];
    free(parent->children);
    parent->children = buff;
    parent->childrenNo--;
}

void appendDir(dirNode* parent, dirNode* child) {
    child->parent = parent;
    dirNode** buff = malloc(sizeof(dirNode*)*(parent->childrenNo+1));
    for(int i=0; i<parent->childrenNo; i++) buff[i] = parent->children[i];
    buff[parent->childrenNo] = child;
    free(parent->children);
    parent->children = buff;
    parent->childrenNo++;
}

