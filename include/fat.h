#ifndef FAT_H
#define FAT_H

#include "defs.h"

bool freeFatBlock(fatentry_t);

void freeChain(fatentry_t);

fatentry_t lastBlockOf(fatentry_t);

fatentry_t getNewBlock(fatentry_t);

#endif // FAT_H
