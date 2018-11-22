#include "../include/fileSys.h"

#define pathErr "Invalid path."

fatentry_t FAT[MAXBLOCKS];
dirNode* directoryHierarchy;
dirNode* workingDir;
MyFILE* currentlyOpen;

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
    block = resetBlock();   block.dir.entryCount = 0;
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
    direntry_t allEntries[(int) (MAXBLOCKS*((BLOCKSIZE-1)/(sizeof(direntry_t)+1)))];
    int entryCount = 0;
    diskblock_t buffBlock;
    for(fatentry_t ptr = FATBLOCKSNO+1; ptr!=ENDOFCHAIN; ptr=FAT[ptr]) {
        readblock(&buffBlock, ptr);
        for(int i=0; i<buffBlock.dir.entryCount; i++) allEntries[entryCount++] = *getEntry(&buffBlock, i);
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
    dirNode* dir[MAXPATHLENGTH];
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
    for(int i=0; i<nodes; i++) entries[i] = initDirEntry(dir[i]->modTime, dir[i]->firstBlock, sizeof(dir[i]->name), dir[i]->childrenNo, dir[i]->name);

    //Overwrite the current directories
    idx = 0;
    diskblock_t buff = resetBlock();
    fatentry_t nextDirBlock;
    size_t buffSize =0;
    for(int i=0; i<nodes; i++) {
        idx++;
        ///TODO: check this part
        buffSize+=sizeof(*entries[i])+entries[i]->nameLength;
        if(buffSize>BLOCKSIZE-(sizeof(short)+2)) {
            buff.dir.entryCount = idx;
            for(int j=idx; j<=0; j--) insertDirEntry(&buff, entries[i-j]);
            idx=0; i--;
            if(i+1-buff.dir.entryCount == 0) nextDirBlock = FATBLOCKSNO+1;
            else if (FAT[nextDirBlock]==ENDOFCHAIN) nextDirBlock = getNewBlock(nextDirBlock);
            else nextDirBlock = FAT[nextDirBlock];
            writeblock(&buff, nextDirBlock);
        }
    }
    if(nextDirBlock!=ENDOFCHAIN) freeChain(nextDirBlock);
}

char* workingDirPath() {
    char path[MAXPATHLENGTH];
    dirNode* dir[MAXPATHLENGTH];
    dir[0] = workingDir;
    int pLength = 0;
    while(dir[pLength]->parent!=NULL) {
        pLength++;
        dir[pLength] = dir[pLength-1]->parent;
    }
    for(int i=pLength; i>=0; i++) {
        strcat(path, "$\\");
        strcat(path, dir[i]->name);
    }
    return strcat(path, ">");
}

void saveVDisk() {
    //writeDirectory first because it can change FAT;
    writeDirectory();
    writeFat();
}

pathStruct parsePath(char* str) {
    pathStruct toRet;
    toRet.isValid = true;
    char* token = NULL;
    dirNode* track;
    int count = 1;
    int nexIdx= 0;
    //count the path components
    token = strtok(str, "\\");
    while(token!=NULL) { token = strtok(NULL, "\\"); count++; }
    token = strtok(str, "\\");
    if(count>MAXPATHLENGTH/2) { toRet.isValid = false; return toRet; }
    toRet.nonExisting   = malloc(sizeof(char*)*(count+1));

    //Check the first token to start tracking the path
    toRet.isAbsolute = false;
    if(strcmp(token, "$")==0) {
        track = directoryHierarchy;
        toRet.isAbsolute = true;
    } else if((strcmp(token, "..")==0) && (workingDir!=directoryHierarchy)) track = workingDir->parent;
    else if(strcmp(token, ".")==0) track = workingDir;
    else {
        for(int i=0; i<workingDir->childrenNo; i++) if(strcmp(token, workingDir->children[i]->name)==0) track = workingDir->children[i];
    }
    if(track!=NULL && track->firstBlock != -1) toRet.isFile = true;
    //Existing part
    while(track!=NULL) {
        //Generate and process new token
        token = strtok(NULL, "\\");
        dirNode* newTrack = NULL;
        //Only a itself, a child or a parent are recognised tokens;
        //why someone would put a ".." or a "." in the middle of a path is beyond me, but even so
        if(strcmp(token, "..")==0) newTrack= track->parent;
        else if(strcmp(token, ".")==0) continue;
        else for(int i=0; i<track->childrenNo; i++) if(strcmp(token, track->children[i]->name)==0) newTrack = track->children[i];
        if(newTrack == NULL) toRet.dir = newTrack;
        track = newTrack;
        if((track!=NULL && track->firstBlock != -1) || (strcmp(token,"$")==0)) {
            if((toRet.isFile)||(strcmp(token, "$")==0)){
                toRet.isValid = false;
                return toRet;
            } else toRet.isFile = true;
        }
    }
    //Non-existing part
    while(token!=NULL) {
        //Record the last token
        toRet.nonExisting[nexIdx] = malloc(sizeof(token));
        strcpy(toRet.nonExisting[nexIdx], token);
        nexIdx++;
        //Generate and process new token
        token = strtok(NULL, "\\");
    }
    toRet.nonExisting[nexIdx] = NULL;

    return toRet;
    ///TODO: pathStructs need to be destructed properly, they use malloc
    ///since many functions use this, maybe make a function that does it
}

void mychdir(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0]!=NULL)) printf(pathErr);
    else workingDir = path.dir;
}

char** mylistdir(dirNode* node) {
    char** ls = malloc(node->childrenNo+1);
    for(int  i=0; i<node->childrenNo; i++) {
        ls[i] = malloc(sizeof(node->children[i]->name));
        strcpy(ls[i], node->children[i]->name);
    }
    ls[node->childrenNo] = NULL;
    return ls;
    ///TODO: the return value must be freed
}
char** mylistpath(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0]!=NULL)) { printf(pathErr); return NULL; }
    else return mylistdir(path.dir);
}

void mymkdir(pathStruct path) {
    ///TODO: needs to check if the names of directories are valid actually
    if(!path.isValid || path.isFile) printf(pathErr);
    dirNode* where = path.dir;
    time(&where->modTime);
    if(path.nonExisting[0]!=NULL) {
        dirNode** buff = where->children;
        where->childrenNo++;
        where->children = malloc(sizeof(dirNode*)*where->childrenNo);
        for(int i=0; i<where->childrenNo-1; i++) where->children[i] = buff[i];
        where->children[where->childrenNo-1] = createNode(path.nonExisting[0], -1, 1, where);
        where = where->children[where->childrenNo-1];
        free(buff);
    }
    for(int i = 1; path.nonExisting[i]!=NULL; i++) {
        where->children[0] = createNode(path.nonExisting[i], -1, 1, where);
        where = where->children[0];
    }
    //leaves a size 1 array; who cares
    where->childrenNo = 0;
}

void myremove (pathStruct path) {
    if(!path.isValid || !path.isFile || path.nonExisting[0]!=NULL) printf(pathErr);
    else {
        removeDir(path.dir->parent, path.dir);
        freeChain(path.dir->firstBlock);
        destroyDir(path.dir);
    }
}

void myrmdir (pathStruct path) {
    if(!path.isValid || path.isFile || path.nonExisting[0]!=NULL) printf(pathErr);
    else {
        //Adjust the parent's parameters
        dirNode* ptr = path.dir->parent;
        time(&ptr->modTime);
        int j = 0;
        while(ptr->children[j]!=path.dir) j++;
        for(int i=j; i<ptr->childrenNo-1; i++) ptr->children[i] = ptr->children[i+1];
        ptr->childrenNo--;
        //delete the directory recursively
        recurRmDir(path.dir);
        destroyDir(path.dir);
    }
}
void recurRmDir( dirNode* dir){
    for(int i=0; i<dir->childrenNo; i++) {
        if(dir->children[i]->firstBlock!=-1) {
            removeDir(dir, dir->children[i]);
            freeChain(dir->children[i]->firstBlock);
        } else recurRmDir(dir->children[i]);
        destroyDir(dir->children[i]);
    }
}

void myMvDir(pathStruct source, pathStruct dest) {
    if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0]!=NULL) printf(pathErr);
    else {
        //Prepare destination
        dirNode* destPtr = dest.dir;
        if(dest.nonExisting[0]!=NULL) {
            mymkdir(dest);
            bool validity = false;
            for(int i=0; i<destPtr->childrenNo; i++) if(strcmp(destPtr->children[i]->name, dest.nonExisting[0])==0) validity = true;
            if(!validity) return;
            int idx =0;
            while(dest.nonExisting[idx]!=NULL) {
                bool changed = false;
                for(int i=0; (i<destPtr->childrenNo) && !changed; i++) if(strcmp(destPtr->children[i]->name, dest.nonExisting[idx])==0) { destPtr = destPtr->children[i]; changed = true; }
            }
        }
        //Prepare source
        dirNode* sourcePtr = source.dir;
        //Make the move
        appendDir(destPtr, sourcePtr);
        removeDir(sourcePtr->parent, sourcePtr);
    }
}

void myCpDir(pathStruct source, pathStruct dest) {
    if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0]!=NULL) printf(pathErr);
    else {
        //Prepare dest
        dirNode* destPtr = dest.dir;
        if(dest.nonExisting[0]!=NULL) {
            mymkdir(dest);
            bool validity = false;
            for(int i=0; i<destPtr->childrenNo; i++) if(strcmp(destPtr->children[i]->name, dest.nonExisting[0])==0) validity = true;
            if(!validity) return;
            int idx =0;
            while(dest.nonExisting[idx]!=NULL) {
                bool changed = false;
                for(int i=0; (i<destPtr->childrenNo) && !changed; i++) if(strcmp(destPtr->children[i]->name, dest.nonExisting[idx])==0) {destPtr = destPtr->children[i]; changed = true; }
            }
        }
        //Make the copy
        for(int i=0; i<source.dir->childrenNo; i++) {
            if(source.dir->firstBlock!=-1) {
                    appendDir(destPtr, createNode(source.dir->name, source.dir->firstBlock, source.dir->childrenNo, NULL));
                    recurCp(source.dir->children[i], destPtr->children[i]);
            } else appendDir(destPtr, cpyFile(source.dir));
        }
    }


}
void recurCp(dirNode* source, dirNode* dest) {
    for(int i=0; i<source->childrenNo; i++) {
        if(source->firstBlock!=-1) {
            appendDir(dest, createNode(source->name, source->firstBlock, source->childrenNo, NULL));
            recurCp(source->children[i], dest->children[i]);
        } else appendDir(dest, cpyFile(source));
    }
}

MyFILE* myfopen (pathStruct path, const char * mode) {
    MyFILE * toOpen = NULL;
    //Identify the file
    dirNode* file;
    if(!path.isValid) { printf(pathErr); return toOpen; }
    if(!path.isFile) {
        if((path.nonExisting[0]!=NULL) && (path.nonExisting[1]==NULL)) {
           dirNode* file = newFile(path.nonExisting[0], path.dir);
           if(file == NULL) return toOpen;
        } else { printf(pathErr); return toOpen; }
    } else file = path.dir;
    //Create the descriptor:
    toOpen = malloc(sizeof(MyFILE));
    memset(toOpen->mode, '\0', 2);
    strcpy(toOpen->mode, mode);
    toOpen->firstBlock = file->firstBlock;
    toOpen->blockNo = lastBlockOf(file->firstBlock);
    readblock(&(toOpen->buffer), toOpen->blockNo);
    int pos =0;
    while(toOpen->buffer.data[pos]!=EOF) pos++;
    toOpen->pos = pos;
    workingDir = file;
    return toOpen;
}

void myfclose() {
    workingDir = workingDir->parent;
    free(currentlyOpen);
    currentlyOpen = NULL;
}
//The writing functions take care of putting EOF at the end
void saveFile() {
    if(strcmp(currentlyOpen->mode, "w")==0) writeblock(&currentlyOpen->buffer, currentlyOpen->blockNo);
    time(&workingDir->modTime);
}

void readFile( pathStruct path ) {
    if(!path.isValid || !path.isFile || path.nonExisting[0]!=NULL) printf(pathErr);
    else {
        char buff[BLOCKSIZE];
        diskblock_t blockB;
        for(fatentry_t bNo = path.dir->firstBlock; bNo!=ENDOFCHAIN; bNo = FAT[bNo]) {
            readblock(&blockB, bNo);
            for(int i=0; i<BLOCKSIZE; i++) buff[i] = (char) blockB.data[i];
            printf("%s", buff);
        }
    }
}

void printLine( int n ) {
    saveFile();
    for(int i = 0; i<n; i++) {
        //Finding the last newline
        currentlyOpen->pos--;
        while(currentlyOpen->buffer.data[currentlyOpen->pos]!='\n') {
            if(currentlyOpen->pos!=0) currentlyOpen->pos--;
            else {
                fatentry_t prev = currentlyOpen->firstBlock;
                while(FAT[prev]!= currentlyOpen->blockNo) prev = FAT[prev];
                readblock(&currentlyOpen->buffer, prev);
                currentlyOpen->blockNo = prev;
                currentlyOpen->pos = BLOCKSIZE-1;
            }
        }
    }
    char buff[BLOCKSIZE];
    for(int j=0; j<BLOCKSIZE; j++) buff[j] = '\0';
    for(int i=0; buff[i]!=EOF; i++){
        if(i==BLOCKSIZE) {
            printf("%s", buff);
            i=0;
            for(int j=0; j<BLOCKSIZE; j++) buff[j] = '\0';
        }
        buff[i] = myfgetc();
    }
    printf("%s", buff);
}

void printFile() {
    char buff[BLOCKSIZE];
    diskblock_t blockB;
    for(fatentry_t bNo = currentlyOpen->firstBlock; bNo!=ENDOFCHAIN; bNo = FAT[bNo]) {
        readblock(&blockB, bNo);
        for(int i=0; i<BLOCKSIZE; i++) buff[i] = (char) blockB.data[i];
        printf("%s", buff);
    }
}

void appendLine( char* line ){
    if(strcmp(currentlyOpen->mode, "w")==0) {
        for(int i=0; line[i]!='\0'; i++) myfputc(line[i]);
        currentlyOpen->buffer.data[currentlyOpen->pos] = EOF;
    } else printf("Permission denied.");
}

void deleteLine(){
    if(strcmp(currentlyOpen->mode, "w")==0) {
        myfremc(); myfremc(); //one for EOF, the other for the last newl
        while(currentlyOpen->buffer.data[currentlyOpen->pos]!='\n') myfremc();
        currentlyOpen->pos++;
        currentlyOpen->buffer.data[currentlyOpen->pos] = EOF;
    } else printf("Permission denied.");
}

char myfgetc () {
    char toRet = currentlyOpen->buffer.data[currentlyOpen->pos];
    if(toRet!=EOF) currentlyOpen->pos++;
    if(currentlyOpen->pos==BLOCKSIZE) {
        currentlyOpen->blockNo = FAT[currentlyOpen->blockNo];
        readblock(&currentlyOpen->buffer, currentlyOpen->blockNo);
        currentlyOpen->pos = 0;
    }
    return toRet;
}

void myfputc (char newc) {
    currentlyOpen->buffer.data[currentlyOpen->pos] = newc;
    currentlyOpen->pos++;
    if(currentlyOpen->pos==BLOCKSIZE) {
        saveFile();
        currentlyOpen->buffer = resetBlock();
        currentlyOpen->pos = 0;
        currentlyOpen->blockNo = getNewBlock(currentlyOpen->blockNo);
        if(currentlyOpen->blockNo == -2) myfclose();
    }
    if(currentlyOpen!=NULL) currentlyOpen->buffer.data[currentlyOpen->pos] = EOF;
}

void myfremc() {
    currentlyOpen->buffer.data[currentlyOpen->pos] = '\0';
    currentlyOpen->pos--;
    if(currentlyOpen->pos==-1){
        if (currentlyOpen->blockNo != currentlyOpen->firstBlock) currentlyOpen->pos =0;
        else {
            freeBlock(currentlyOpen->blockNo);
            freeFatBlock(currentlyOpen->blockNo);
            fatentry_t blockNo = currentlyOpen->firstBlock;
            while(FAT[FAT[blockNo]]!=UNUSED) blockNo = FAT[blockNo];
            FAT[blockNo] = ENDOFCHAIN;
            currentlyOpen->blockNo = blockNo;
            currentlyOpen->pos = BLOCKSIZE-1;
            readblock(&currentlyOpen->buffer, currentlyOpen->blockNo);
        }
    }
    currentlyOpen->buffer.data[currentlyOpen->pos] = EOF;
}

bool isDiskFile(FILE* source){
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

dirNode* newFile(char* name, dirNode* parent){
    dirNode* file = NULL;
    fatentry_t ptr = getNewBlock(-1);
    if(ptr!=-2) {
        diskblock_t blk = resetBlock();
        blk.data[0] = EOF;
        writeblock(&blk, ptr);
        file = createNode(name, ptr, -1, parent);
    } return file;
}

dirNode* cpyFile(dirNode* file) {
    dirNode* toRet = NULL;
    int counter = 1;
    fatentry_t ptr = file->firstBlock;
    for(; FAT[ptr]!=ENDOFCHAIN; ptr = FAT[ptr]) counter++;

    fatentry_t newFBlock = getNewBlock(-1);
    ptr = newFBlock;
    for(int i=0; (i<counter)&&(ptr!=-2); i++) ptr = getNewBlock(ptr);
    if(ptr!=-2) {
        char* name = "Copy of ";
        strcat(name, file->name);
        toRet = createNode(name, newFBlock, -1, NULL);
        diskblock_t buff;
        ptr = newFBlock;
        fatentry_t sourcePtr = file->firstBlock;
        while(sourcePtr!=ENDOFCHAIN) {
            readblock(&buff, sourcePtr);
            writeblock(&buff, ptr);
            ptr = FAT[ptr];
            sourcePtr = FAT[sourcePtr];
        }
    } else freeChain(newFBlock);
    return toRet;
}
