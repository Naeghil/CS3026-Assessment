#include "../include/fat.h"

extern fatentry_t FAT[MAXBLOCKS];          //maintain the FAT
diskblock_t virtualDisk[MAXBLOCKS];

bool freeFatBlock(fatentry_t idx) {
    if (idx<=FATBLOCKSNO) return false;
    FAT[idx] = UNUSED;
    return true;
}

void freeChain(fatentry_t start) {
    fatentry_t ptr = start;
    fatentry_t rem;
    while (ptr!= ENDOFCHAIN) {
        rem = ptr;
        ptr = FAT[ptr];
        FAT[rem] = UNUSED;
    }
    FAT[ptr] = UNUSED;
}

fatentry_t lastBlockOf(fatentry_t start) {
    fatentry_t toRet = start;
    while(FAT[toRet]!=ENDOFCHAIN) toRet = FAT[toRet];
    return toRet;
}

fatentry_t getNewBlock(fatentry_t from) {
    fatentry_t toRet = (from>FATBLOCKSNO) ? from+1 : FATBLOCKSNO+1;
    while((toRet<MAXBLOCKS)&&(FAT[toRet]=!UNUSED)) toRet++;
    if(FAT[toRet]!=UNUSED) {
            toRet = FATBLOCKSNO+1;
            while((toRet<from-1)&&(FAT[toRet]!=UNUSED)) toRet++;
    }
    if(FAT[toRet]!= UNUSED) { printf("Memory full."); return -2; }

    if(FAT[from]!=ENDOFCHAIN) FAT[from] = toRet;
    FAT[toRet] = ENDOFCHAIN;
    return toRet;
}

