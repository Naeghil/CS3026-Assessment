#include "../include/fileSys.h"

#define pathErr "Invalid path.\n"

fatentry_t FAT[MAXBLOCKS];
dirNode* directoryHierarchy;
dirNode* workingDir;
MyFILE* opened;
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
    for(int i=0; i<MAXARGS; i++) memset(root.nonExisting[i], '\0', MAXNAME);
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
        buff.dir[i%DIRENTRYCOUNT] = initDirEntry(dir[i]->modTime, dir[i]->fBlock, dir[i]->childrenNo, dir[i]->name);
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
    for(int i=0; i<MAXARGS; i++) memset(toRet.nonExisting, '\0', MAXNAME);
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
        if(token==NULL);
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
        if((newTrack!=NULL)&&(newTrack->fBlock!=-1)) {
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
    while((token!=NULL)&&(nexIdx<MAXARGS)) {
        //Record the last token
        strcat(toRet.nonExisting[nexIdx], token);
        nexIdx++;
        //Generate and process new token
        token = strtok(NULL, "/");
    }
    if(nexIdx==MAXARGS) { printf("Will operate on the first 16 non-existing elements\n"); nexIdx--; }
    return toRet;
 }

void mychdir(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0][0]!='\0')) printf(pathErr);
    else workingDir = path.dir;
}

char* mylistdir(dirNode* node) {
    char buff[MAXNAME*MAXBLOCKS];
    memset(buff, '\0', MAXNAME*MAXBLOCKS);
    for(int i = 0; i<node->childrenNo; i++) {
        strcat(buff, node->children[i]->name);
        strcat(buff, "\t");
    }
    char* ls = malloc(strlen(buff)+1);
    memset(ls, '\0', strlen(buff)+1);
    for(int i=0; i<(strlen(buff)+1); i++) ls[i]=buff[i];
    return ls;
}
char* mylistpath(pathStruct path) {
    if(!path.isValid || path.isFile || (path.nonExisting[0][0]!='\0')) { printf(pathErr); return NULL; }
    else return mylistdir(path.dir);
}

void mymkdir(pathStruct path) {
    if(!path.isValid || path.isFile) printf(pathErr);
    else {
        dirNode* where = path.dir;
        if(path.nonExisting[0][0]=='\0') {
                printf("A directory by this name already exists.\n");
                return;
        }
        if(path.nonExisting[0][0]!='\0') {
            time(&where->modTime);
            if((strchr(path.nonExisting[0], '\'')!=NULL)||(strchr(path.nonExisting[0], ' ')!=NULL) || (strchr(path.nonExisting[0], '\"')!=NULL)||(strchr(path.nonExisting[0], '_')!=NULL)) {
                printf("Invalid name. \n");
                return;
            }
            appendDir(where, createNode(path.nonExisting[0], -1, 1, where));
            where = where->children[where->childrenNo-1];
        }
        for(int i = 1; path.nonExisting[i][0]!='\0'; i++) {
            if((strchr(path.nonExisting[i], '\'')!=NULL)||(strchr(path.nonExisting[0], ' ')!=NULL)|| (strchr(path.nonExisting[0], '_')!=NULL)) {
                printf("Invalid name. \n");
                return;
            }
            where->children[0] = createNode(path.nonExisting[i], -1, 1, where);
            where = where->children[0];
        }
        //leaves a size 1 array; who cares
        where->childrenNo = 0;
    }
}

void myremove (pathStruct path) {
    if(!path.isValid || !path.isFile || path.nonExisting[0][0]!='\0') printf(pathErr);
    else {
        removeDir(path.dir->parent, path.dir);
        freeChain(path.dir->fBlock);
        free(path.dir);
    }
}

void myrmdir (pathStruct path) {
    if(!path.isValid || path.isFile || path.nonExisting[0][0]!='\0') printf(pathErr);
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
        free(path.dir);
    }
}
void recurRmDir( dirNode* dir){
    for(int i=0; i<dir->childrenNo; i++) {
        if(dir->children[i]->fBlock!=-1) {
            removeDir(dir, dir->children[i]);
            freeChain(dir->children[i]->fBlock);
        } else recurRmDir(dir->children[i]);
        free(dir->children[i]);
    }
}

void myMvDir(pathStruct source, pathStruct dest) {
   if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0][0]!='\0') printf(pathErr);
    else if ((source.dir->parent == dest.dir) && (dest.nonExisting[0][0]!='\0') && (dest.nonExisting[1][0]=='\0'))  {
        memset(source.dir->name, '\0', MAXNAME);
        strcat(source.dir->name, dest.nonExisting[0]);
        memset(dest.nonExisting, '\0', MAXNAME);
    } else {
        //Prepare destination
        dirNode* destPtr = dest.dir;
        if(dest.nonExisting[0][0]!='\0') {
            mymkdir(dest);
            while(destPtr->childrenNo!=0) destPtr = destPtr->children[destPtr->childrenNo-1];
        }
        //Prepare source
        dirNode* sourcePtr = source.dir;
        //Make the move
        removeDir(sourcePtr->parent, sourcePtr);
        appendDir(destPtr, sourcePtr);
    }
}

void myCpDir(pathStruct source, pathStruct dest) {
    if(!source.isValid || !dest.isValid || dest.isFile || source.nonExisting[0][0]!='\0') printf(pathErr);
    else {
        //Prepare ptrs
        dirNode* destPtr = dest.dir;
        if(dest.nonExisting[0][0]!='\0') {
            mymkdir(dest);
            while(destPtr->childrenNo!=0) destPtr = destPtr->children[destPtr->childrenNo-1];
        }
        dirNode* srcPtr = source.dir;
        //Make the copy
        char* name;
        if(srcPtr->parent == destPtr) {
            name = malloc(sizeof(srcPtr->name)+6);
            strcpy(name, "Copyof");
            strcat(name, srcPtr->name);
        }
        else name = srcPtr->name;
        if(srcPtr->fBlock==-1){
            appendDir(destPtr, createNode(name, srcPtr->fBlock, 0, destPtr));
            recurCp(srcPtr, destPtr);
        } else appendDir(destPtr, cpyFile(name, srcPtr));
        printf("%d\n", destPtr->children[destPtr->childrenNo-1]->fBlock);
    }
}

void recurCp(dirNode* source, dirNode* dest) {
    for(int i=0; i<source->childrenNo; i++) {
        dirNode* ptr = source->children[i];
        if(source->fBlock!=-1) {
            appendDir(dest, createNode(ptr->name, ptr->fBlock, 0, dest));
            recurCp(ptr, dest->children[i]);
        } else appendDir(dest, cpyFile(source->name, source));
    }
}

MyFILE* myfopen (pathStruct path, const char * mode) {
    MyFILE * toOpen = NULL;
    //Identify the file
    dirNode* file = path.dir;
    if(!path.isValid) { printf(pathErr); return toOpen; }
    if(!path.isFile) {
        char filename[MAXNAME];
        memset(filename, '\0', MAXNAME);
        int count = 0;
        while(path.nonExisting[count][0]!='\0') count++;
        strcat(filename, path.nonExisting[count-1]);
        memset(path.nonExisting[count-1], '\0', MAXNAME);
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
    toOpen->fBlock = file->fBlock;
    toOpen->blockNo = lastBlockOf(file->fBlock);
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
    free(opened);
    opened = NULL;
}
//The writing functions take care of putting EOF at the end
void saveFile() {
    if(strcmp(opened->mode, "w")==0) writeblock(&opened->buffer, opened->blockNo);
    time(&workingDir->modTime);
}

void readFile( pathStruct path ) {
    if(!path.isValid || !path.isFile || path.nonExisting[0][0]!='\0') printf(pathErr);
    else {
        char buff[BLOCKSIZE];
        diskblock_t blockB;
        for(fatentry_t bNo = path.dir->fBlock; bNo!=ENDOFCHAIN; bNo = FAT[bNo]) {
            readblock(&blockB, bNo);
            for(int i=0; i<BLOCKSIZE; i++) if((char) blockB.data[i]!=EOF) buff[i] = (char) blockB.data[i];
            printf("%s", buff);
        }
    }
}

void printLine( int n ) {
    saveFile();
    if((opened->blockNo==opened->fBlock)&&(opened->pos==0)) return;
    for(int i = 0; i<n; i++) {
       //Finding the last newline
        int pos = opened->pos-2;
        while(opened->buffer.data[pos]!='\n'){
            if(pos!=0) pos--;
            else if(opened->blockNo != opened->fBlock){
                fatentry_t prev = opened->fBlock;
                while(FAT[prev]!= opened->blockNo) prev = FAT[prev];
                readblock(&(opened->buffer), prev);
                opened->blockNo = prev;
                pos = BLOCKSIZE-1;
            } else break;

        }
        opened->pos = pos;
        if((opened->blockNo==opened->fBlock)&&(opened->pos==0)) break;
    }
    char buff = myfgetc();
    if(buff=='\n') buff = myfgetc();
    while(buff!=EOF) {
        printf("%c", buff);
        buff = myfgetc();
    } opened->pos--;
}

void printFile() {
    printLine((MAXBLOCKS-4)*BLOCKSIZE);
}

void appendLine( char* line ){
    if(strcmp(opened->mode, "w")==0) {
        for(int i=0; line[i]!='\0'; i++) if(line[i]!='\"') myfputc(line[i]);
        myfputc('\n');
        opened->buffer.data[opened->pos] = EOF;
    } else printf("Permission denied.\n");
}

void deleteLine(){
    if(strcmp(opened->mode, "w")==0) {
    if((opened->blockNo==opened->fBlock)&&(opened->pos==0)) return;
        myfremc(); myfremc(); //one for EOF, the other for the last newl
        while((char) opened->buffer.data[opened->pos]!='\n'){
            myfremc();
            if((opened->blockNo==opened->fBlock)&&(opened->pos==0)) {
                opened->pos--;
                break;
            }
        }
        opened->pos++;
        opened->buffer.data[opened->pos] = EOF;
    } else printf("Permission denied.\n");
}

char myfgetc () {
    char toRet = opened->buffer.data[opened->pos];
    opened->pos++;
    if((opened->pos==BLOCKSIZE)&&(opened->buffer.data[opened->pos]!=EOF)) {
        opened->blockNo = FAT[opened->blockNo];
        readblock(&opened->buffer, opened->blockNo);
        opened->pos = 0;
    }
    return toRet;
}

void myfputc (char newc) {
    opened->buffer.data[opened->pos] = newc;
    opened->pos++;
    if(opened->pos==BLOCKSIZE) {
        saveFile();
        opened->buffer = resetBlock();
        opened->pos = 0;
        opened->blockNo = getNewBlock(opened->blockNo);
        if(opened->blockNo == -2) myfclose();
    }
    if(opened!=NULL) opened->buffer.data[opened->pos] = EOF;
}

void myfremc() {
    opened->buffer.data[opened->pos] = '\0';
    opened->pos--;
    if(opened->pos==-1){
        if (opened->blockNo != opened->fBlock) opened->pos = 0;
        else {
            freeBlock(opened->blockNo);
            freeFatBlock(opened->blockNo);
            fatentry_t blockNo = opened->fBlock;
            while(FAT[FAT[blockNo]]!=UNUSED) blockNo = FAT[blockNo];
            FAT[blockNo] = ENDOFCHAIN;
            opened->blockNo = blockNo;
            opened->pos = BLOCKSIZE-1;
            readblock(&opened->buffer, opened->blockNo);
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

dirNode* cpyFile(char* name, dirNode* file) {
    dirNode* toRet = NULL;
    //Check how much blocks are needed
    int counter = 1;
    fatentry_t ptr = file->fBlock;
    for(; FAT[ptr]!=ENDOFCHAIN; ptr = FAT[ptr]) counter++;
    //Check if there are enough blocks available
    fatentry_t newPtr = getNewBlock(-1);
    ptr = newPtr;
    for(int i=1; i<counter; i++) {
        if(ptr!=-2) ptr = getNewBlock(ptr);
        else {
            freeChain(newPtr);
            return toRet;
        }
    }
    freeChain(newPtr);
    //Create the new file and init the copying process
    toRet = newFile(name, NULL);
    newPtr = toRet->fBlock;
    diskblock_t buff = resetBlock();
    //Copy
    for(ptr = file->fBlock; ptr!=ENDOFCHAIN; ptr = FAT[ptr]) {
        readblock(&buff, ptr);
        writeblock(&buff, newPtr);
        newPtr = getNewBlock(newPtr);
    }
    return toRet;
}
