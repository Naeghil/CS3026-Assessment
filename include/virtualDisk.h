#ifndef VIRTUALDISK_H
#define VIRTUALDISK_H

#include <stdio.h>
#include "defs.h"

/// the disk is a list of diskblocks (shared in the program)
extern diskblock_t virtualDisk [ MAXBLOCKS ] ;

bool writedisk ( const char * );
void readdisk ( const char * );
void writeblock( diskblock_t* , int );
void readblock ( diskblock_t* , int );

///Utilities:
diskblock_t resetBlock();

#endif // VIRTUALDISK_H
