#include "../include/virtualDisk.h"

diskblock_t virtualDisk[MAXBLOCKS];

// writedisk : writes virtual disk out to physical disk
bool writedisk ( const char * filename ) {
    bool success = true;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 ) success = false;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;
   return success;
}

// readdisk: reads virtual disk from file ??
void readdisk ( const char * filename ) {
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
}

void writeblock ( diskblock_t * block, int block_address ) {
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}

void readblock ( diskblock_t* block, int block_address ) {
    memmove( block->data, &virtualDisk[block_address], BLOCKSIZE );
}

 diskblock_t resetBlock(){
    diskblock_t blk;
    for(int i=0; i<BLOCKSIZE; i++) blk.data[i] = '\0';
    return blk;
}

void freeBlock(fatentry_t idx) {
    diskblock_t blk = resetBlock();
    writeblock(&blk, idx); }
