#include "../include/shell.h"

///Extern declarations:
bool session;
extern pathStruct root;
dirNode* workingDir;
MyFILE* opened;

///Error Messages:
#define argErr      "Invalid arguments.\n"
#define flagErr     "%s is not a recognised flag.\n"
#define noOpenFile  "No file is currently open.\n"
#define fileOpen    "A file is currently open. Type 'close' to close it.\n"
#define notSaved    "The file could not be saved.\n"

///The manual entries:
char* manual[AVAILABLECMDS] = {
"man [COMMAND] \n"
    "\tDescribes COMMAND. The first line shows the format in terms of:\n"
        "\t\tcommand [OPTIONAL_ARGUMENT] ALTERNATIVE_ARGUMENT1|alternative_exact_text1 ARGUMENT exact_text\n"
    "\tand subsequently provides a description for the command and its arguments.\n"
    "\tCOMMAND:\n"
        "\t\tOne of the available commands; all the commands are described if no argument is provided",
"quit \n"
    "\t Exits the shell. DOES NOT save the disk\n"
"save [PATH]\n"
    "\tTries to save the virtual disk to PATH.\n"
    "\tPATH:\n"
        "\t\tA valid path on your computer, can be absolute or relative.\n"
        "\t\tIn case it's not provided, the shell will save to './vDisk', overwriting any previously saved disk.",
"load DISKPATH \n"
    "\tLoads a previously saved disk from DISKPATH. It doesn't save the current disk. \n"
    "\tDISKPATH:\n"
        "\t\tA valid path to a previously saved disk on your computer.\n",
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
    "\tCopies the file or empty folder from SOURCE to the directory specified by DESTINATION.\n"
    "\tSOURCE:\n"
        "\t\tA valid path on the virtual disk. It can be absolute or relative.\n"
    "\tDESTINATION:\n"
        "\t\tA valid path on the virtual disk pointing to a directory.\n",
"open [-r|-w] PATH\n"
    "\tOpens the specified file or creates it if it doesn't already exist, effectively 'changing the current directory into it'.\n"
    "\tIf no flag is specified, the file will open in -w.\n"
    "\tPATH:\n"
        "\t\tA valid path on the virtual disk that points to a file.\n"
    "\t-r:\n"
        "\t\tWhen this flag is provided only the commands 'rd' and 'rl' are available for the file (readonly mode)\n"
    "\t-w:\n"
        "\t\tWhen this flag is provided the commands 'rd', 'rl', wl' and 'rml' will be available for the file (write/read/append mode)",
"close [-e]\n"
    "\tCloses the currently open file if there is one, and tries and save its contents to the virtual disk.\n"
    "\t-e:\n"
        "\t\tWhen this flag is provided, the last changes not automatically saved won't be saved to disk.",
"sf\n"
    "\tTries to save the contents of the open file to the virtual disk.",
"rd [PATH]\n"
    "\tPrints all the contents of the currently open file or one specified in PATH. It doesn't open the file.\n"
    "\tPATH:\n"
        "\t\tA valid path on the virtual disk pointing to a file.\n"
        "\t\tIts specification does allow to check the content of a specified file even if another file is already open.",
"rl [NO]\n"
    "\tPrints the last NO lines of the currently open file, or the whole file.\n"
    "\tNO:\n"
        "\t\tAn integer.",
"wl [STRING]\n"
    "\tAppends a newline containing STRING to the currently open file.\n"
    "\tSTRING:\n"
        "\t\tA sequence of characters enclosed in \".",
"rml\n"
    "\tRemoves the last line of a currently open file."
};

void execute(commandStruct cmd) {
    int command = getCmdIdx(cmd.command);
    int argc = cmd.argNumber;
    //man
    if(command==0)  {
        if(argc>1) printf(argErr);
        else if(argc==1) {
            int idx=getCmdIdx(cmd.arguments[0]);
            if(idx!=-1) printf("%s \n\n", manual[idx]);
        } else for(int i=0; i<AVAILABLECMDS; i++) printf("%s \n\n", manual[i]);
    }
    //quit
    else if(command==1)  {
        if(opened!=NULL) {printf(fileOpen); return; }
        if(argc!=0) { printf(argErr); return; }
        session = false;
    }
    //save
    else if(command==2)  {
        if(argc>1) { printf(argErr); return; }
        if(opened!=NULL) {printf(fileOpen); return; }
        char* path = (argc==1) ? cmd.arguments[0] : "vDisk";
        saveVDisk();
        if(!writedisk(path)) printf("The disk couldn't be saved to file. \n");
    }
    //load
    else if(command==3)  {
        if(opened!=NULL) {printf(fileOpen); return; }
        if(argc!=1) printf(argErr);
        else {
            readdisk(cmd.arguments[0]);
            initStructs();
        }
    }
    //cd
    else if(command==4)  {
        if(opened!=NULL) {printf(fileOpen); return; }
        if(argc>1) printf(argErr);
        else if(argc==1) mychdir(parsePath(cmd.arguments[0]));
        else mychdir(root);
    }
    //ls
    else if(command==5)  {
        if(opened!=NULL) {printf(fileOpen); return; }
        char* contents;
        if(argc>1) printf(argErr);
        else if(argc==1) contents = mylistpath(parsePath(cmd.arguments[0]));
        else contents = mylistdir(workingDir);
        printf("%s\n", contents);
    }
    //mkdir
    else if(command==6)  {
        if(opened!=NULL) {printf(fileOpen); return; }
        if(argc!=1) printf(argErr);
        else mymkdir(parsePath(cmd.arguments[0]));
    }
    //rm
    else if(command==7)  {
        if(opened!=NULL) {printf(fileOpen); return; }
        if((argc==0)||(argc>2)) printf(argErr);
        else {
            pathStruct path;
            if(argc==1) {
                    path = parsePath(cmd.arguments[0]);
                    if(path.isFile) myremove(path);
                    else if(path.dir->childrenNo==0) myrmdir(path);
                    else printf("The directory is not empty!\n");
            } else if(strcmp(cmd.arguments[0], "-f")==0) {
                    path = parsePath(cmd.arguments[1]);
                    if(path.isFile) myremove(path);
                    else myrmdir(path);
            } else printf(flagErr, cmd.arguments[0]);
        }
    }
    //mv
    else if(command==8)  {
        if(opened!=NULL) {printf(fileOpen); return; }
        if(argc!=2) printf(argErr);
        else myMvDir(parsePath(cmd.arguments[0]), parsePath(cmd.arguments[1]));
    }
    //cp
    else if(command==9) {
        if(opened!=NULL) {printf(fileOpen); return; }
        if(argc!=2) printf(argErr);
        else myCpDir(parsePath(cmd.arguments[0]), parsePath(cmd.arguments[1]));
    }
    //open
    else if(command==10) {
        if(opened!=NULL) {printf(fileOpen); return; }
        if((argc==0)||(argc>2)) printf(argErr);
        else {
            if((argc==1)||(strcmp(cmd.arguments[0], "-w")==0)) opened = myfopen(parsePath(cmd.arguments[argc-1]), "w");
            else if (strcmp(cmd.arguments[0], "-r")==0) opened = myfopen(parsePath(cmd.arguments[argc-1]), "r");
            else printf(flagErr, cmd.arguments[0]);
        }
    }
    //close
    else if(command==11) {
        if(argc>1) printf(argErr);
        else if(opened == NULL) printf(noOpenFile);
        else if(argc==0) {
            saveFile();
            myfclose();
        } else if(strcmp(cmd.arguments[0], "-e")==0) myfclose();
        else printf(flagErr, cmd.arguments[0]);
    }
    //sf
    else if(command==12) {
        if(argc!=0) printf(argErr);
        else if (opened == NULL) printf(noOpenFile);
        else saveFile();
    }
    //rd
    else if(command==13) {
        if((opened==NULL)||(argc==1)) {
            if(argc!=1) printf(argErr);
            else readFile(parsePath(cmd.arguments[0]));
        } else {
            if(argc!=0) printf(argErr);
            else printFile();
        }
    }
    //rl
    else if(command==14) {
        if(argc>1) printf(argErr);
        else if(opened == NULL) printf(noOpenFile);
        else if(argc==0) printLine(1);
        else {
            for(int i=0; i<strlen(cmd.arguments[0]); i++) if((cmd.arguments[0][i]<'1')||(cmd.arguments[0][i]>'9')) {
                printf("The argument is not a number.\n");
                return;
            }
            printLine(atoi(cmd.arguments[0]));
        }
    }
    //wl
    else if(command==15) {
        if(argc!=1) printf(argErr);
        else if(opened == NULL) printf(noOpenFile);
        else appendLine(cmd.arguments[0]);
    }
    //rml
    else if(command==16){
        if(argc!=0) printf(argErr);
        else if(opened == NULL) printf(noOpenFile);
        else deleteLine();
    }
}
