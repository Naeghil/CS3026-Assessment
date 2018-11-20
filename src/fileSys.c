#include "fileSys.h"

fatentry_t FAT[MAXBLOCKS];
fileSys* directoryHierarchy;

///TODO: Is this needed?
//maintain the number of entries per block to speed up entry retrieval
//This works like a two-level indexing: when an entry is needed, iterate through this array
//and subtract the values each time
//an entry has a minimum sie of 14b and a maximum size of 394b; the medium is of
directoryTable      secondLvlDirIdx;


void format () {
    diskblock_t block = resetBlock();
    ///Reset all memory:
    for (int i=FATBLOCKSNO+1; i<MAXBLOCKS; i++) writeblock(&block, i);
    // block0; the comparison disk says "CS3026 Operating Systems Assessment 2012"
    strcpy((char*)(unsigned char*)block.data, "CS3026 Operating Systems Assessment");
    writeblock(&block, 0);
    ///Prepare FAT
    block = resetBlock();
    for(int i=0; i<FATENTRYCOUNT; i++) block.fat[i] = UNUSED;
    for(int i=2; i<=FATBLOCKSNO; i++) writeblock(&block, i);
    for (short i=1; i<FATBLOCKSNO; i++) block.fat[i] = i+1;
    block.fat[FATBLOCKSNO] = ENDOFCHAIN;
    block.fat[0] = ENDOFCHAIN;
    block.fat[FATBLOCKSNO+1] = ENDOFCHAIN; //for root directory
    writeblock(&block, 1);
    ///Prepare the directory block
    block = initDirBlock(0);
    time_t now; time(&now);
    direntry_t *root = initDirEntry(now, FATBLOCKSNO+1, strlen(""), 0, "");
    insertDirEntry(&block, root);
    writeblock(&block, FATBLOCKSNO);
}

void initStructs(){
    ///Initialize fat:
    diskblock_t fatBlocks[FATBLOCKSNO];
    for(int i=0; i<FATBLOCKSNO; i++) readblock(&fatBlocks[i], i+1);
    for(int i=0; i<MAXBLOCKS; i++) FAT[i] = fatBlocks[(int) i/FATENTRYCOUNT].fat[i%FATENTRYCOUNT];

    ///Initialize directory hierarchy:
    int entryCount;
    diskblock_t buffBlock;
    readblock(&buffBlock, lastBlockOf(FATBLOCKSNO+1));
    entryCount = buffBlock.dir.nextId + buffBlock.dir.entryCount;
    direntry_t allEntries[entryCount];
    fatentry_t ptr = FATBLOCKSNO+1;
    readblock(&buffBlock, ptr);
    for(int i=0; i<entryCount; i++) {
        if(i==(buffBlock.dir.nextId+buffBlock.dir.entryCount)) {
            ptr = FAT[ptr];
            readblock(&buffBlock, ptr);
        }
        allEntries[i] = *getEntry(&buffBlock, i);
    }

    //TODO: initialize directory hierarchy to root
    // make a creator function to make nodes that takes in direntries

}

void writeFat() {
    diskblock_t fatBlock = resetBlock();
    for (int i=0; i<FATBLOCKSNO; i++) {
        for(int j=0; j<FATENTRYCOUNT; j++) fatBlock.fat[j] = FAT[i*FATENTRYCOUNT+j];
        writeblock(&fatBlock, i+1);
    }
}
/*
file* myfopen(const char* fName, const char* mode) {
    file* toOpen = malloc(sizeof(file));
    memset(toOpen->mode, '\0', sizeof(char)*3);
    strcpy(toOpen->mode, mode);
    toOpen->buffer = resetBlock();
    //dirManagement utility that looks for a file and "opens" it
    //setting firstBlock, blockNo and pos
    //openFile(fName, toOpen);
    //dirManagement utility that registers a new entry for a new file
    //sets toOpen to null if there's no memory left
    //if (toOpen->blockNo == UNUSED) newFile(fName, toOpen);

     this goes in the thing that looks for free blocks?
    if (toOpen->blockno == -1) {
        errorHandle(-4);
        return NULL;
    }
    return toOpen;
}

void myfclose(file* toClose) {

    1. if not "w", just free the memory and close the file
    2. otherwise, add EOF and write out the buffer to virtualDisk[blockno]


}

int myfgetc (file * toRead) {
    //Access error must be dealt with at a higher level: if(strchr(toRead->mode, 'r') == NULL) return -5;
    int retChar = toRead->buffer.data[toRead->pos];
    if (retChar == EOF) return retChar;
    toRead->pos++;
    if (toRead->pos==BLOCKSIZE) {
        toRead->pos=0;
        toRead->blockNo = FAT[toRead->blockNo];
    };
    return retChar;
}

void myfputc (int b, file * toWrite){
    //Access error must be dealt with at a higher level
    //Out of Memory error must be dealt with at a higher level
    toWrite->pos++;
    if (toWrite->pos == BLOCKSIZE) {
        writeblock(&toWrite->buffer, toWrite->blockNo);
//        toWrite->blockNo = getFreeBlock(toWrite->blockNo);
        toWrite->pos = 0;
    }
    toWrite->buffer.data[toWrite->pos] = (Byte) b;
}
*/
bool isFileDisk(FILE* source){
    fseek(source, 0 , SEEK_END);
    long fileSize = ftell(source);
    if(fileSize == sizeof(virtualDisk)) {
            return true;
            fseek(source, 0 , SEEK_SET);
    } else return false;
}

void loadDiskFromFile(FILE* source){
    ///TODO: write the file to the virtualDisk
}
