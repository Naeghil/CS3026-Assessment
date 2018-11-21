#include "../include/shell.h"

///Extern declarations:
bool session;
pathStruct root;
fileSys* workingDir;
MyFILE* currentlyOpen;

///Error Messages:
const char* argErr = "Invalid arguments.";
const char* flagErr = "%s is not a recognised flag.";
const char* noOpenFile = "No file is currently open.";
const char* fileOpen    = "A file is currently open.";
const char* notSaved = "The file could not be saved.";

///The manual entries:
char* manual[AVAILABLECMDS] = {
"man [COMMAND] \n"
    "\tDescribes COMMAND. The first line shows the format in terms of:\n"
        "\t\tcommand [OPTIONAL_ARGUMENT] ALTERNATIVE_ARGUMENT1|alternative_exact_text1 ARGUMENT exact_text\n"
    "\tand subsequently provides a description for the command and its arguments.\n"
    "\tCOMMAND:\n"
        "\t\tOne of the available commands; all the commands are described if no argument is provided",
"quit [-f] [PATH] \n"
    "\t Exits the shell and tries to save the virtual disk to PATH.\n"
    "\tPATH:\n"
        "\t\tsee 'save'\n"
    "\t-f:\n"
        "\t\tIf this flag is provided, the command will close the shell even if the disk couldn't be saved.",
"save [PATH]\n"
    "\tTries to save the virtual disk to PATH.\n"
    "\tPATH:\n"
        "\t\tA valid path on your computer, can be absolute or relative.\n"
        "\t\tIn case it's not provided, the shell will save to './vDisk', overwriting any previously saved disk.",
"load DISKPATH [SAVEPATH]\n"
    "\tLoads a previously saved disk from DISKPATH and tries to save the current disk in SAVEPATH.\n"
    "\tDISKPATH:\n"
        "\t\tA valid path to a previously saved disk on your computer.\n"
    "\tSAVEPATH:\n"
        "\t\tsee PATH in 'save'",
"cd [PATH]\n"
    "\tChanges the directory to the specified PATH.\n"
    "\tPATH:\n"
        "\t\tA valid path on the virtual disk. If not specified, the directory will change to root.\n"
        "\t\tIf PATH is '..' the directory will change to the parent of the current directory.",
"ls [PATH]\n"
    "\tLists the contents of the specified directory.\n"
    "\tPATH:\n"
        "\t\tA valid path on the virtual disk.\n"
        "\t\tIf not specified, it will list the contents of '.', that is the current directory.",
"mkdir PATH\n"
    "\tCreates new directories as specified by PATH.\n"
    "\tPATH:\n"
        "\t\tCan consist of an existing part and a non existing part.\n"
        "\t\tThe existing part must be a valid absolute or relative path.\n"
        "\t\tThe non existing part can consist of any level of subdirectories that will be automatically created.",
"rm [-f] PATH\n"
    "\tDeletes the specified file or directory.\n"
    "\t-f:\n"
        "\t\tThis flag has no effect if deleting a file.\n"
        "\t\tIf not provided, only empty directories can be deleted.\n"
        "\t\tWhen provided while deleting a directory, any subdirectory or file contained in it will be recursively deleted.\n"
    "\tPATH:\n"
        "\t\tA valid path on the virtual disk. It can be absolute or relative.",
"mv SOURCE DESTINATION\n"
    "\tMoves the file or directory from SOURCE to DESTINATION.\n"
    "\tSOURCE:\n"
        "\t\tA valid path on the virtual disk. It can be absolute or relative.\n"
    "\tDESTINATION:\n"
        "\t\tA valid path on the virtual disk.\n"
        "\t\tIf SOURCE points to a file or subdirectory contained in the same directory specified by DESTINATION, the command effectively renames the file or subdirectory.",
"cp SOURCE DESTINATION\n"
    "\tCopies the file or directory from SOURCE to the directory specified by DESTINATION.\n"
    "\tSOURCE:\n"
        "\t\tA valid path on the virtual disk. It can be absolute or relative.\n"
    "\tDESTINATION:\n"
        "\t\tA valid path on the virtual disk pointing to a directory.\n"
        "\t\tIf SOURCE points to a file or subdirectory contained in the same directory specified by DESTINATION, the command has no effect.",
"open [-r|-w] PATH\n"
    "\tOpens the specified file or creates it if it doesn't already exist, effectively 'changing the current directory into it'.\n"
    "\tIf no flag is specified, the file will open in -w.\n"
    "\tPATH:\n"
        "\t\tA valid path on the virtual disk that points to a file.\n"
    "\t-r:\n"
        "\t\tWhen this flag is provided only the commands 'rd' and 'rl' are available for the file (readonly mode)\n"
    "\t-w:\n"
        "\t\tWhen this flag is provided the commands 'rd', 'rl', wl' and 'rml' will be available for the file (write/read/append mode)",
"close [-f|-e]\n"
    "\tCloses the currently open file if there is one, and tries and save its contents to the virtual disk.\n"
    "\t-f:\n"
        "\t\tWhen this flag is provided, the file will be closed even if its contents couldn't be saved.\n"
    "\t-e:\n"
        "\t\tWhen this flag is provided, the last changes not automatically saved won't be saved to disk.",
"sf\n"
    "\tTries to save the contents of the open file to the virtual disk.",
"rd [PATH]\n"
    "\tPrints all the contents of the currently open file or one specified in PATH. It doesn't open the file.\n"
    "\tPATH:\n"
        "\t\tA valid path on the virtual disk pointing to a file.\n"
        "\t\tIts specification does allow to check the content of a specified file even if another file is already open.",
///TODO: it might be useful to check the last n lines of a non open file, but very low priority
"rl [NO]\n"
    "\tPrints the last NO lines of the currently open file.\n"
    "\tNO:\n"
        "\t\tAn integer.",
"wl [STRING]\n"
    "\tAppends a line containing STRING to the currently open file.\n"
    "\tSTRING:\n"
        "\t\tA sequence of characters enclosed in '. If it contains \\n, multiple lines will be written.",
"rml\n"
    "\tRemoves the last line of a currently open file."
};


///Command Functions:
void printManual(int argc, char** argv) {
    if(argc>1) printf(argErr);
    else if(argc==1) {
            int idx=getCmdIdx(argv[0]);
            if(idx!=-1) printf("%s", manual[idx]);
    } else for(int i=0; i<AVAILABLECMDS; i++) printf("%s \n\n", manual[i]);
};
    ///Session commands
//exits the shell but tries and save the disk to file; -f doesn't save the disk
void quit(int argc, char** argv){
    if((argc>2)||((argc==2)&&(strcmp(argv[0],"-f")!=0))) { printf(argErr); return; }
    char* path = "vDisk";
    const char* errMessage = "The disk couldn't be saved to file. \n";
    saveVDisk();
    if((argc==2)||(strcmp(argv[0], "-f")!=0)) path = argv[argc-1];
    session = !writedisk(path);
    if(session&&((argc==0)||(strcmp(argv[0], "-f")!=0))) printf(errMessage);
};
//saves the disk to file
void save(int argc, char** argv){
    if(argc>1) { printf(argErr); return; }
    if(currentlyOpen!=NULL) {printf(fileOpen); return; }
    char* path = (argc==1) ? argv[0] : "vDisk";
    saveVDisk();
    if(!writedisk(path)) printf("The disk couldn't be saved to file. \n");
};
//loads a disk from file
void load(int argc, char** argv){
    if((argc<1)||(argc>2)) printf(argErr);
    else {
        char* newArgv[] = {argv[argc-1]};
        save(argc-1, newArgv);
        FILE * source = fopen(argv[0], "rb");
        if(source) {
            if(isDiskFile(source)) loadDiskFromFile(source);
            else printf("The file is not of the right dimension.");
            fclose(source);
        } else printf("The file cannot be found.");
    }
};
    ///Directory commands
//changes the working directory to the specified directory; to root if no argument provided
void cd(int argc, char** argv){
    if(argc>1) printf(argErr);
    ///TODO: implement parsePath and the hierachy tree
    else if(argc==1) mychdir(parsePath(argv[0]));
    else mychdir(root);
};
//lists the contents of the directory specified; the current if no arguments provided
void ls(int argc, char** argv){
    char** contents;
    if(argc>1) printf(argErr);
    else if(argc==1) contents = mylistdir(parsePath(argv[0]));
    else contents = mylistdir(getPath(workingDir));
    for(int i=0; contents[i]!=NULL; i++) printf("%s  ", contents[i]);
};
//creates a new directory (together with specified subdirectories); can accept absolute path
void mkDir(int argc, char** argv){
    if(argc!=1) printf(argErr);
    else mymkdir(parsePath(argv[0]));
};
//removes a file or directory (-f removes all subdirectories too)
void rm(int argc, char** argv){
    if((argc==0)||(argc>2)) printf(argErr);
    else {
        pathStruct *path = NULL;
        if(argc==1) {
                *path = parsePath(argv[0]);
                if(path->isFile) myremove(*path);
                else if(path->isEmpty) myrmdir(*path);
                else printf("The directory is not empty!");
        } else if(strcmp(argv[0], "-f")==0) {
                *path = parsePath(argv[1]);
                if(path->isFile) myremove(*path);
                else myrmdir(*path);
        } else printf(flagErr, argv[0]);
    }
};
//moves a file/directory to a specified directory; also used to rename stuff
void mv(int argc, char** argv){
    if(argc!=2) printf(argErr);
    else myMvDir(parsePath(argv[0]), parsePath(argv[1]));
};
//copies a file/directory to a specified directory
void cp(int argc, char** argv){
    if(argc!=2) printf(argErr);
    else myCpDir(parsePath(argv[0]), parsePath(argv[1]));
};
    ///File editing commands
//opens an existing file or creates a new one if it doesn't exist
void myOpen(int argc, char** argv){
    if((argc==0)||(argc>2)) printf(argErr);
    else {
        if((argc==1)||(strcmp(argv[0], "-w")==0)) currentlyOpen = myfopen(parsePath(argv[argc-1]), "w");
        else if (strcmp(argv[0], "-r")==0) currentlyOpen = myfopen(parsePath(argv[argc-1]), "r");
        else printf(flagErr, argv[0]);
    }
};
//closes the open file and saves its contents to disk
void closef(int argc, char** argv){
    if(argc>1) printf(argErr);
    else if(currentlyOpen == NULL) printf(noOpenFile);
    else if(argc==0) {
        if(saveFile()) myfclose();
        else printf(notSaved);
    } else if(strcmp(argv[0], "-f")==0) {
        saveFile();
        myfclose();
    } else if(strcmp(argv[0], "-e")==0) myfclose();
    else printf(flagErr, argv[0]);
};
//saves the contents of an open file to the virtual disk
void savef(int argc, char** argv){
    if(argc!=0) printf(argErr);
    else if(!saveFile()) printf(notSaved);
};
// prints to screen the contents of an open file, or a specified file without opening it
void myRead(int argc, char** argv){
    if((currentlyOpen==NULL)||(argc==1)) {
        if(argc!=1) printf(argErr);
        else readFile(parsePath(argv[0]));
    } else {
        if(argc!=0) printf(argErr);
        else printFile();
    };
};
// reads the last line of an open file
void readl(int argc, char** argv){
    if(argc>1) printf(argErr);
    else if(currentlyOpen == NULL) printf(noOpenFile);
    else if(argc==0) printLine(0);
    ///TODO:check if the character is a number, does atoi do that?
    else printLine(atoi(argv[0]));
};
// appends a line to an open file
void writel(int argc, char** argv){
    if(argc>1) printf(argErr);
    else if(currentlyOpen == NULL) printf(noOpenFile);
    else appendLine(argv[0]);
};
// removes the last line of an open file
void removel(int argc, char** argv){
    if(argc!=0) printf(argErr);
    else if(currentlyOpen == NULL) printf(noOpenFile);
    else deleteLine();
};
