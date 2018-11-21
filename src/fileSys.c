#include "../include/fileSys.h"

fatentry_t FAT[MAXBLOCKS];
fileSys* directoryHierarchy;

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
    free(root);
    writeblock(&block, FATBLOCKSNO);
}

//Used to initialise structures maintained at runtime
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
    directoryHierarchy = makeDirTree(allEntries, entryCount);
}

void writeFat() {
    diskblock_t fatBlock;
    for (int i=0; i<FATBLOCKSNO; i++) {
        fatBlock = resetBlock();
        for(int j=0; j<FATENTRYCOUNT; j++) fatBlock.fat[j] = FAT[i*FATENTRYCOUNT+j];
        writeblock(&fatBlock, i+1);
    }
}

void writeDirectory() {
    //Make a sequence of entries in the way they need to be stored
    fileSys* dir[MAXPATHLENGTH];
    int nodes = 0;
    int idx =1;
    direntry_t* entries[MAXPATHLENGTH];
    dir[0] = directoryHierarchy;
    while(nodes<=idx) {
        for(int i=0; i<dir[nodes]->childrenNo; i++) {
            dir[idx] = dir[nodes]->children[i];
            idx++;
        }
        nodes++;
    }
    for(int i=0; i<nodes; i++) entries = initDirEntry(dir[i]->modTime, dir[i]->firstBlock, signed char (sizeof(dir[i]->name)), dir[i]->childrenNo, dir[i]->name);

    //Overwrite the current directories
    idx = 0;
    diskblock_t buff = resetBlock();
    fatentry_t lastDirBlock = FATBLOCKSNO+1;
    size_t buffSize =0;
    for(int i=0; i<nodes; i++) {
        //count the entries and fill the buffSize
        //when it overflows buffSize overflows, write the block and get a new one;
            //if (FAT[lastDirBlock]==ENDOFCHAIN) lastDirBlock = getNewBlock(lastDirBlock);
            //else lastDirBlock = FAT[lastDirBlock];
    }

    //if more are needed, get them
    //this probably introduces the "getFreeCell" function from the fat.h
        //getNewBlock(fatentry_t from): if from is not -1, set FAT[from] to toRet; set FAT[toRet] to ENDOFCHAIN and return it;
    //just get the first free one
    //lastly, check if less directories are needed, in which case the fat needs to be modified accordingly

}

char* workingDirPath() {
    char path[MAXPATHLENGTH];
    fileSys* dir[MAXPATHLENGTH];
    dir[0] = workingDir;
    int pLength = 0;
    while(dir[pLength]->parent!=NULL) {
        pLength++;
        dir[pLength] = dir[pLength-1]->parent;
    }
    for(int i=pLength; i>=0; i++) {
        strcat(path, "$\\")
        strcat(path, dir[i]->name);
    }
    return strcat(path, ">");
}

void saveVDisk() {
    //writeDirectory first because it can change FAT;
    writeDirectory();
    writeFat();
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
