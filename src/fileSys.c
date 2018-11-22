#include "../include/fileSys.h"

#define pathErr "Invalid path."

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
    block = resetBlock();   block.entryCount = 0;
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
    for(int i=0; i<nodes; i++) entries[i] = initDirEntry(dir[i]->modTime, dir[i]->firstBlock, signed char (sizeof(dir[i]->name)), dir[i]->childrenNo, dir[i]->name);

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

pathStruct parsePath(const char* str) {
    pathStruct toRet;
    toRet.isValid = true;
    char* token = NULL;
    fileSys* track;
    int count = 1;
    int exIdx = 0;
    int nexIdx= 0;
    //count the path components
    token = strtok(str, "\\");
    while(token!=NULL) { token = strtok(NULL, "\\"); count++; }
    token = strtok(str, "\\");
    if(count>MAXPATHLENGTH/2) { toRet.isValid = false; return toRet; }
    toRet.existing      = malloc(sizeof(fileSys*)*(count+1));
    toRet.nonExisting   = malloc(sizeof(char*)*(count+1));

    //Check the first token to start tracking the path
    toRet.isAbsolute = false;
    if(strcmp(token, "$")==0) {
        track = directoryHierarchy;
        toRet.isAbsolute = true;
    } else if((strcmp(token, "..")==0) && (workingDir!=directoryHierarchy)) track = workingDir->parent;
    else if(strcmp(token, ".")==0) track = workingDir;
    else {
        for(int i=0; i<workingDir->childrenNo; i++) if(strcmp(token, workingDir->children[i])==0) track = workingDir->children[i];
    }
    if(track!=NULL && track->firstBlock != -1) toRet.isFile = true;
    //Existing part
    while(track!=NULL) {
        //Record the last token
        toRet.existing[exIdx] = track;
        exIdx++;
        //Generate and process new token
        token = strtok(NULL, "\\");
        fileSys* newTrack = NULL;
        //Only a itself, a child or a parent are recognised tokens;
        //why someone would put a ".." or a "." in the middle of a path is beyond me, but even so
        for(int i=0; i<track->childrenNo; i++) if(strcmp(token, track->children[i])==0) newTrack = track->children[i];
        if(strcmp(token, "..")==0) newTrack= track->parent;
        if(strcmp(token, ".")==0) { exIdx--; continue; }
        track = newTrack;
        if((track!=NULL && track->firstBlock != -1) || (strcmp(token=="$")==0)) {
            if((toRet.isFile)||(strcmp(token, "$")==0){
                toRet.isValid = false;
                return toRet;
            }
            else toRet.isFile = true;
        }
    }
    toRet.existing[exIdx] = NULL;
    while(token!=NULL) {
        //Record the last token
        toRet.nonExisting[nexIdx] = malloc(sizeof(token));
        strcpy(toRet.nonExisting[nexIdx], token);
        nexIdx++;
        //Generate and process new token
        token = strtok(NULL, "\\");
        //validation of non-existing tokens should be handled by the relevant functions
    }
    toRet[nexIdx] = NULL;

    return toRet;
    ///TODO: pathStructs need to be destructed properly, they use malloc
    ///since many functions use this, maybe make a function that does it
}

void mychdir(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0]!=NULL)) printf(pathErr);
    else {
            int idx =0;
            while(path.existing[idx]!=NULL) idx++;
            workingDir = path.existing[idx-1];
    }
}

char** mylistdir(fileSys* node) {
    char* ls[node->childrenNo+1];
    for(int  i=0; i<node->childrenNo; i++) {
        ls[i] = malloc(sizeof(node->children[i]->name));
        strcpy(ls[i], node->children[i]);
    }
    ls[node->chilrdenNo] = NULL;
    return ls;
}
char** mylistdir(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0]!=NULL)) { printf(pathErr); return NULL; }
    else {
            int idx = 0;
            while(path.existing[idx]!=NULL) idx++;
            return mylistdir(path->existing[idx-1]);
    }
}

void mymkdir(pathStruct path) {
    ///TODO: needs to check if the names of directories are valid actually
    if(!path.isValid || path.isFile) printf(pathErr);
    int idx = 0;
    while(path.existing[idx]!=NULL) idx++;
    fileSys* where = path.existing[idx-1];
    time_t now; time(now);
    where->modTime = now;
    if(path.nonExisting[0]!=NULL) {
        fileSys** buff = where->children;
        where->childrenNo++;
        where->children = malloc(sizeof(fileSys*)*where->childrenNo);
        for(int i=0; i<where->childrenNo-1; i++) where->children[i] = buff[i];
        where->children[where->childrenNo-1] = makeNode(path.nonExisting[idx], now, -1, 1);
        where = where->children[where->childrenNo-1];
        free(buff);
    }
    for(idx = 1; path.nonExisting[idx]!=NULL; idx++) {
        where->children[0] = makeNode(path.nonExisting[idx], now, -1, 1);
        where = where->children[0];
    }
    //leave a size1 array; who cares
    where->childrenNo = 0;

}

void myremove (pathStruct path) {
    if(!path.isValid || !path.isFile || path.nonExisting[0]!=NULL) printf(pathErr);
    else {
        int idx;
        for(idx = 0; path.existing[idx]!=NULL; idx++);
        //actually frees diskblocks and modifies fat
        //it also deals with the parent children parameters (including modtime)
        deleteFromDisk(path.existing[idx-1]);
    }
}

void myrmdir (pathStruct path) {
    if(!path.isValid || path.isFile || path.nonExisting[0]!NULL) printf(pathErr);
    else {
        int idx;
        for(idx; path.existing[idx]!=NULL; idx++);
        //Adjust the parent's parameters
        fileSys* ptr = path.existing[idx-1]->parent;
        time_t now; time(now);  ptr->modTime = now;
        int i = 0;
        while(ptr->children[i]!=path.existing[idx-1]) i++;
        for(i; i<ptr->childrenNo-1; i++) ptr->children[i] = ptr->children[i+1];
        ptr->childrenNo--;
        //delete the directory recursively
        recurRmDir(path.existing[idx-1]);
        free(path.existing[idx-1]);

    }
}
void recurRmDir( fileSys* dir){
    for(int i=0; i<dir->childrenNo; i++) {
        if(dir->children[i]->firstBlock!=-1) deleteFromDisk(dir->children[i]);
        else recurRmDir(dir->children[i]);
        free(dir->children[i]);
    }
    free(dir->children);
    free(dir->name);
}

void myMvDir(pathStruct source, pathStruct dest) {
    if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0]!NULL) printf(pathErr);
    else {
        //Prepare destination
        fileSys* destPtr;
        for(int i=0; dest.existing[i]!=NULL; i++) destPtr = dest.existing[i];
        if(dest.nonExisting[0]!=NULL) {
            mymkdir(dest);
            bool validity = false;
            for(int i=0; i<destPtr->childrenNo; i++) if(strcmp(destPtr->children[i], dest.nonExisting[0])==0) validity = true;
            if(!validity) return;
            int idx =0;
            while(dest.nonExisting[idx]!=NULL) {
                bool changed = false;
                for(int i=0; (i<destPtr->childrenNo) && !changed; i++) if(strcmp(destPtr->children[i], dest.nonExisting[idx])==0) { destPtr = destPtr->children[i]; changed = true; }
            }
        }
        //Prepare source
        fileSys* sourcePtr;
        for(int i=0; source.existing[i]!=NULL; i++) sourcePtr = dest.existing[i];
        //Make the move
        appendDir(destPtr, sourcePtr);
        removeDir(sourcePtr->parent, sourcePtr);
        ///TODO: consider making a function that appends a child to a fileSys and another that removes it
    }
}

void myCpDir(pathStruct source, pathStruct dest) {
    if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0]!NULL) printf(pathErr);
    else {
        //Prepare dest
        fileSys* destPtr = dest.dir;
        if(dest.nonExisting[0]!=NULL) {
            mymkdir(dest);
            bool validity = false;
            for(int i=0; i<destPtr->childrenNo; i++) if(strcmp(destPtr->children[i], dest.nonExisting[0])==0) validity = true;
            if(!validity) return;
            int idx =0;
            while(dest.nonExisting[idx]!=NULL) {
                bool changed = false;
                for(int i=0; (i<destPtr->childrenNo) && !changed; i++) if(strcmp(destPtr->children[i], dest.nonExisting[idx])==0) {destPtr = destPtr->children[i]; changed = true; }
            }
        }
        //Make the copy
        time_t now; time(now);
        for(int i=0; i<source.dir->childrenNo; i++) {
            if(source.dir->firstBlock!=-1) {
                    appendDir(destPtr, makeNode(source.dir->name, now, source.dir->firstBlock, source.dir->childrenNo));
                    recurCp(source.dir->children[i], destPtr->children[i]);
            } else appendDir(destPtr, cpyFile(source.dir));
        }
    }


}
void recurCp(fileSys* source, fileSys* dest) {
    time_t now; time(now);
    for(int i=0; i<source->childrenNo; i++) {
        if(source->firstBlock!=-1) {
            appendDir(dest, makeNode(source->name, now, source->firstBlock, source->childrenNo));
            recurCp(source->children[i], dest->children[i]);
        } else appendDir(dest, cpyFile(source));
    }
}

//Opens a file, manages a buffer[BLOCKSIZE]
//mode: "r" readonly; "w" read/write/append
MyFILE* myfopen (pathStruct path, const char * mode) {
    //Identify the file
    fileSys* file;
    if(!path.isValid) { printf(pathErr); return; }
    if(!path.isFile) {
        if((path.nonExisting[0]!=NULL) && (path.nonExisting[1]==NULL) {
           ///TODO: creates a new dirNode for a file, with parent path.dir
           ///This also clears the block and adds an EOF for the data
           fileSys* file = newFile(path.nonExisting[0], path.dir);
           if(file == NULL) return NULL;
        } else { printf(pathErr); return NULL; }
    } else file = path.dir;
    //Create the descriptor:
    MyFILE * toOpen = malloc(sizeof(MyFILE));
    memset(toOpen->mode, '\0', 2);
    strcpy(toOpen->mode, mode);
    toOpen->blockNo = lastBlockOf(file->fistBlock);
    readblock(&(toOpen->buffer), toOpen->blockNo);
    int pos =0;
    while(toOpen->buffer.data[pos]!=EOF) pos++;
    toOpen->pos = pos;
    workingDir = file;
    return toOpen;
}

void myfclose() {
    workingDir = workingDir->parent;
    free(currentlyOpen->buffer);
    free(currentlyOpen);
    currentlyOpen = NULL;
}

bool saveFile() {
    ///TODO: Implement this
}

/*
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
