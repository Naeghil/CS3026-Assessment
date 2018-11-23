#include "../include/fileSys.h"

#define pathErr "Invalid path.\n"

fatentry_t FAT[MAXBLOCKS];
dirNode* directoryHierarchy;
dirNode* workingDir;
MyFILE* currentlyOpen;
pathStruct root;

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
    block = resetBlock();
    time_t now; time(&now);
    direntry_t root = initDirEntry(now, -1, 0, "root");
    block.dir[0] = root;
    writeblock(&block, FATBLOCKSNO+1);
}

//Used to initialise structures maintained at runtime
void initStructs(){
    ///Initialize fat:
    diskblock_t fatBlocks[FATBLOCKSNO];
    for(int i=0; i<FATBLOCKSNO; i++) readblock(&fatBlocks[i], i+1);
    for(int i=0; i<MAXBLOCKS; i++) FAT[i] = fatBlocks[(int) i/FATENTRYCOUNT].fat[i%FATENTRYCOUNT];

    ///Initialize directory hierarchy:
    direntry_t allEntries[MAXBLOCKS*DIRENTRYCOUNT];
    diskblock_t buffBlock = resetBlock();
    int idx =0;
    for(fatentry_t ptr = FATBLOCKSNO+1; ptr!=ENDOFCHAIN; ptr=FAT[ptr]) {
        readblock(&buffBlock, ptr);
        for(int i=0; (buffBlock.dir[i].modtime!=0) && (i<DIRENTRYCOUNT); i++) allEntries[idx++] = buffBlock.dir[i];
    }
    directoryHierarchy = makeDirTree(allEntries, idx);
    directoryHierarchy->parent = NULL;
    workingDir = directoryHierarchy;
    root.isFile = false; root.isValid = true;
    root.nonExisting[0] = NULL;
    root.dir = directoryHierarchy;
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
    dirNode* dir[MAXBLOCKS*DIRENTRYCOUNT];
    int nodes = 0;
    int idx =1;
    dir[0] = directoryHierarchy;

    while(nodes<idx) {
        for(int i=0; i<dir[nodes]->childrenNo; i++) {
            dir[idx] = dir[nodes]->children[i];
            idx++;
        } nodes++;
    }

    //Overwrite the current directories
    idx = 0;
    diskblock_t buff = resetBlock();
    fatentry_t nextDirBlock = nextDirBlock = FATBLOCKSNO+1;
    for(int i=0; i<nodes; i++) {
        buff.dir[i%DIRENTRYCOUNT] = initDirEntry(dir[i]->modTime, dir[i]->firstBlock, dir[i]->childrenNo, dir[i]->name);
        if((i%DIRENTRYCOUNT)==(DIRENTRYCOUNT-1)) {
            writeblock(&buff, nextDirBlock);
            buff = resetBlock();
            if(FAT[nextDirBlock]==ENDOFCHAIN) nextDirBlock = getNewBlock(nextDirBlock);
            else nextDirBlock = FAT[nextDirBlock];
        }
    }
    writeblock(&buff, nextDirBlock);
    if(FAT[nextDirBlock]!=ENDOFCHAIN) freeChain(FAT[nextDirBlock]);
    FAT[nextDirBlock] = ENDOFCHAIN;
}

char* workingDirPath() {
    char path[MAXPATHLENGTH];
    memset(path, '\0', MAXPATHLENGTH);
    dirNode* dir[MAXPATHLENGTH];
    dir[0] = workingDir;
    int pLength = 0;
    while(strcmp(dir[pLength]->name, directoryHierarchy->name)!=0) {
        pLength++;
        dir[pLength] = dir[pLength-1]->parent;
    }
    strcat(path, "$");
    for(int i=pLength-1; i>=0; i--) {
        strcat(path, "\\");
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
    toRet.isFile = false;
    toRet.dir = NULL;
    char* token = NULL;
    dirNode* track;

    if(str[0]=='/') track = directoryHierarchy;
    else track = workingDir;
    //Existing part
    for(token = strtok(str, "/"); track!=NULL;) {
        dirNode* newTrack = NULL;
        if(token==NULL) track ==NULL;
        else if(strcmp(token, ".")==0) {
                token = strtok(NULL,"/");
                continue;
        } else if(strcmp(token, "..")==0) {
            if(track!=directoryHierarchy) newTrack = track->parent;
            else {
                toRet.isValid = false;
                return toRet;
            }
        } else for(int i=0; i<track->childrenNo; i++)
            if(strcmp(token, track->children[i]->name)==0) newTrack = track->children[i];
        if((newTrack!=NULL)&&(newTrack->firstBlock!=-1)) {
            if(toRet.isFile){
                toRet.isValid = false;
                return toRet;
            } else toRet.isFile = true;
        }
        if(newTrack == NULL) toRet.dir = track;
        track = newTrack;
        if(track!=NULL) token = strtok(NULL, "/");
    }
    //Non-existing part
    int nexIdx= 0;
    while((token!=NULL)&&(nexIdx<16)) {
        //Record the last token
        toRet.nonExisting[nexIdx] = malloc(sizeof(token));
        strcpy(toRet.nonExisting[nexIdx], token);
        nexIdx++;
        //Generate and process new token
        token = strtok(NULL, "/");
    }
    if(nexIdx==16) { printf("Will operate on the first 15 non-existing elements\n"); nexIdx--; }
    toRet.nonExisting[nexIdx] = NULL;
    return toRet;
}

void mychdir(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0]!=NULL)) printf(pathErr);
    else workingDir = path.dir;
}

char* mylistdir(dirNode* node) {
    int count=0;
    for(int i = 0; i<node->childrenNo; i++) count+=sizeof(node->children[i]->name);
    char* ls = malloc(count+1);
    memset(ls, '\0', count+1);
    if(node->childrenNo>0) strcat(ls, node->children[0]->name);
    for(int  i=1; i<node->childrenNo; i++) {
        strcat(ls, "\t");
        strcat(ls, node->children[i]->name);
    }
    return ls;
}
char* mylistpath(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0]!=NULL)) { printf(pathErr); return NULL; }
    else return mylistdir(path.dir);
}

void mymkdir(pathStruct path) {
    if(!path.isValid || path.isFile) printf(pathErr);
    else {
        dirNode* where = path.dir;
        if(path.nonExisting[0]==NULL) {
                printf("A directory by this name already exists.\n");
                return;
        }
        if(path.nonExisting[0]!=NULL) {
            time(&where->modTime);
            if((strchr(path.nonExisting[0], '\'')!=NULL)||(strchr(path.nonExisting[0], ' ')!=NULL) || (strchr(path.nonExisting[0], '\"')!=NULL)||(strchr(path.nonExisting[0], '_')!=NULL)) {
                printf("Invalid name. \n");
                return;
            }
            appendDir(where, createNode(path.nonExisting[0], -1, 1, where));
            where = where->children[where->childrenNo-1];
        }
        for(int i = 1; path.nonExisting[i]!=NULL; i++) {
            if((strchr(path.nonExisting[i], '\'')!=NULL)||(strchr(path.nonExisting[0], ' ')!=NULL)|| (strchr(path.nonExisting[0], '_')!=NULL)) {
                printf("Invalid name. \n");
                return;
            }
            where->children[0] = createNode(path.nonExisting[i], -1, 1, where);
            where = where->children[0];
        }
        //leaves a size 1 array; who cares
        where->childrenNo = 0;
        for(int i=0; path.nonExisting[i]!=NULL; i++) free(path.nonExisting[i]);
    }
}

void myremove (pathStruct path) {
    if(!path.isValid || !path.isFile || path.nonExisting[0]!=NULL) printf(pathErr);
    else {
        removeDir(path.dir->parent, path.dir);
        freeChain(path.dir->firstBlock);
        destroyDir(path.dir);
    }
    for(int i=0;  path.nonExisting[i]!=NULL; i++) free(path.nonExisting[i]);
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
    for(int i=0; path.nonExisting[i]!=NULL; i++) free(path.nonExisting[i]);
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
    bool changed = false;
    if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0]!=NULL) printf(pathErr);
    else if ((source.dir->parent == dest.dir) && (dest.nonExisting[0]!=NULL) && (dest.nonExisting[1]==NULL))  {
        free(source.dir->name);
        source.dir->name = dest.nonExisting[0];
        dest.nonExisting[0] = NULL;
    } else {
        //Prepare destination
        dirNode* destPtr = dest.dir;
        if(dest.nonExisting[0]!=NULL) {
            mymkdir(dest);
            changed = true;
            while(destPtr->childrenNo!=0) destPtr = destPtr->children[destPtr->childrenNo-1];
        }
        //Prepare source
        dirNode* sourcePtr = source.dir;
        //Make the move
        removeDir(sourcePtr->parent, sourcePtr);
        appendDir(destPtr, sourcePtr);
    }
    if(!changed) for(int i=0; dest.nonExisting[i]!=NULL; i++) free(dest.nonExisting[i]);
}

void myCpDir(pathStruct source, pathStruct dest) {
    bool changed = false;
    if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0]!=NULL) printf(pathErr);
    else {
        //Prepare ptrs
        dirNode* destPtr = dest.dir;
        if(dest.nonExisting[0]!=NULL) {
            mymkdir(dest);
            changed = true;
            while(destPtr->childrenNo!=0) destPtr = destPtr->children[destPtr->childrenNo-1];
        }
        dirNode* srcPtr = source.dir;
        //Make the copy
        char* name;
        if(srcPtr->parent == destPtr) {
            name = malloc(sizeof(srcPtr->name)+8);
            strcpy(name, "Copy of ");
            strcat(name, srcPtr->name);
        }
        else name = srcPtr->name;
        appendDir(destPtr, createNode(name, srcPtr->firstBlock, 0, destPtr));
        destPtr = destPtr->children[destPtr->childrenNo-1];

        for(int i=0; i<srcPtr->childrenNo; i++) {
            dirNode* ptr = srcPtr->children[i];
            if(ptr->firstBlock!=-1) {
                appendDir(destPtr, createNode(ptr->name, ptr->firstBlock, 0, destPtr));
                recurCp(ptr, destPtr->children[i]);
            } else appendDir(destPtr, cpyFile(ptr));
        }
    }
    if(!changed) for(int i=0; dest.nonExisting[i]!=NULL; i++) free(dest.nonExisting[i]);
}
void recurCp(dirNode* source, dirNode* dest) {
    for(int i=0; i<source->childrenNo; i++) {
        dirNode* ptr = source->children[i];
        if(source->firstBlock!=-1) {
            appendDir(dest, createNode(ptr->name, ptr->firstBlock, 0, dest));
            recurCp(ptr, dest->children[i]);
        } else appendDir(dest, cpyFile(source));
    }
}

MyFILE* myfopen (pathStruct path, const char * mode) {
    MyFILE * toOpen = NULL;
    //Identify the file
    dirNode* file = path.dir;
    if(!path.isValid) { printf(pathErr); return toOpen; }
    if(!path.isFile) {
        char* filename;
        int count;
        for(count =0; path.nonExisting[count]!=NULL; count++ ) filename = path.nonExisting[count];
        path.nonExisting[count-1] = NULL;
        if(count>1) {
            mymkdir(path);
            while(file->childrenNo!=0) file = file->children[file->childrenNo-1];
        }
        file = newFile(filename, file);
        if(file == NULL) return toOpen;
        else appendDir(file->parent, file);
    }
    //Create the descriptor:
    toOpen = malloc(sizeof(MyFILE));
    memset(toOpen->mode, '\0', 2);
    strcpy(toOpen->mode, mode);
    toOpen->firstBlock = file->firstBlock;
    toOpen->blockNo = lastBlockOf(file->firstBlock);
    diskblock_t buffBuff = resetBlock();
    readblock(&buffBuff, (toOpen->blockNo));
    toOpen->buffer = buffBuff;
    int pos = 0;
    while((char) toOpen->buffer.data[pos]!=EOF) pos++;
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
            for(int i=0; i<BLOCKSIZE; i++) if((char) blockB.data[i]!=EOF) buff[i] = (char) blockB.data[i];
            printf("%s", buff);
        }
    }
    for(int i=0; path.nonExisting[i]!=NULL; i++) free(path.nonExisting[i]);
}

void printLine( int n ) {
    saveFile();

    for(int i = 0; i<n; i++) {
       //Finding the last newline
        int pos = currentlyOpen->pos-2;
        while(currentlyOpen->buffer.data[pos]!='\n'){
            if(pos!=0) pos--;
            else if(currentlyOpen->blockNo != currentlyOpen->firstBlock){
                fatentry_t prev = currentlyOpen->firstBlock;
                while(FAT[prev]!= currentlyOpen->blockNo) prev = FAT[prev];
                readblock(&(currentlyOpen->buffer), prev);
                currentlyOpen->blockNo = prev;
                pos = BLOCKSIZE-1;
            } else break;

        }
        currentlyOpen->pos = pos;
        if((currentlyOpen->blockNo==currentlyOpen->firstBlock)&&(currentlyOpen->pos==0)) break;
    }
    char buff = myfgetc();
    if(buff=='\n') buff = myfgetc();
    while(buff!=EOF) {
        printf("%c", buff);
        buff = myfgetc();
    } currentlyOpen->pos--;
    printf("\n");
}

void printFile() {
    printLine((MAXBLOCKS-4)*BLOCKSIZE);
}

void appendLine( char* line ){
    if(strcmp(currentlyOpen->mode, "w")==0) {
        for(int i=0; line[i]!='\0'; i++) if(line[i]!='\"') myfputc(line[i]);
        myfputc('\n');
        currentlyOpen->buffer.data[currentlyOpen->pos] = EOF;
    } else printf("Permission denied.\n");
}

void deleteLine(){
    if(strcmp(currentlyOpen->mode, "w")==0) {
    if((currentlyOpen->blockNo==currentlyOpen->firstBlock)&&(currentlyOpen->pos==0)) return;
        myfremc(); myfremc(); //one for EOF, the other for the last newl
        while((char) currentlyOpen->buffer.data[currentlyOpen->pos]!='\n'){
            printf("%d, %d, %d\n", currentlyOpen->pos, currentlyOpen->blockNo, (char) currentlyOpen->buffer.data[currentlyOpen->pos]);
            myfremc();
            if((currentlyOpen->blockNo==currentlyOpen->firstBlock)&&(currentlyOpen->pos==0)) {
                currentlyOpen->pos--;
                break;
            }
        }
        currentlyOpen->pos++;
        currentlyOpen->buffer.data[currentlyOpen->pos] = EOF;
    } else printf("Permission denied.\n");
}

char myfgetc () {
    char toRet = currentlyOpen->buffer.data[currentlyOpen->pos];
    currentlyOpen->pos++;
    if((currentlyOpen->pos==BLOCKSIZE)&&(currentlyOpen->buffer.data[currentlyOpen->pos]!=EOF)) {
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
        if (currentlyOpen->blockNo != currentlyOpen->firstBlock) currentlyOpen->pos = 0;
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
}

bool isDiskFile(FILE* source){
    fseek(source, 0 , SEEK_END);
    long fileSize = ftell(source);
    if(fileSize == sizeof(virtualDisk)) {
            return true;
            fseek(source, 0 , SEEK_SET);
    } else return false;
}

dirNode* newFile(char* name, dirNode* parent){
    dirNode* file = NULL;
    fatentry_t ptr = getNewBlock(-1);
    if(ptr!=-2) {
        diskblock_t blk = resetBlock();
        blk.data[0] = EOF;
        writeblock(&blk, ptr);
        file = createNode(name, ptr, -1, parent);
    }
    return file;
}

dirNode* cpyFile(dirNode* file) {
    ///TODO: apparently this gives a dirNode with the same fBlock
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
