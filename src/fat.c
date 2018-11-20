#include "fat.h"

extern fatentry_t FAT[MAXBLOCKS];          //maintain the FAT

bool freeBlock(fatentry_t idx, fatentry_t FAT[MAXBLOCKS]) {
    if (idx<=FATBLOCKSNO) return false;
    FAT[idx] = UNUSED;
    return true;
}

bool freeChain(fatentry_t start, fatentry_t FAT[MAXBLOCKS]) {
    bool possible = true;
    fatentry_t ptr = start;
    while (ptr!= ENDOFCHAIN) {
        ///TODO: finish implementation
    }
    return possible;
}

fatentry_t lastBlockOf(fatentry_t start) {
    fatentry_t toRet = start;
    while(FAT[toRet]!=ENDOFCHAIN) toRet = FAT[toRet];
    return toRet;
}