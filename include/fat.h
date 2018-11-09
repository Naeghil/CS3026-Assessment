#ifndef FAT_H
#define FAT_H

#include "defs.h"
extern fatentry_t FAT[MAXBLOCKS];

bool freeBlock(fatentry_t, fatentry_t*);

bool freeChain(fatentry_t, fatentry_t*);


#endif // FAT_H
