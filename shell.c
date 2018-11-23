#include <stdio.h>
#include <stdlib.h>

#include "include/shell.h"

extern bool session;
dirNode* workingDir;

int main(int argc, char * argv[]) {
    initialize();
    while (session) execute(getCommand());

    return 0;
}

void initialize() {
    session = true;
    printf("Initializing memory...\n");
    format();
    printf("Memory initialized.\n");
    printf("Initializing runtime structures...\n");
    initStructs();
    printf("Runtime structures initialized.\n");
    printf("Type in 'man' for details on available commands\n");
}

commandStruct getCommand() {
    char* commandString = malloc(MAXCMD);
    printf(workingDirPath());
    fgets(commandString, MAXCMD, stdin);
    commandStruct cmd = parse(commandString);
    free(commandString);
    return cmd;
}

void execute(commandStruct cmd) {
    int command = getCmdIdx(cmd.command);
    if(command==0)  printManual(cmd.argNumber, cmd.arguments);
    ///Session commands
    else if(command==1)  quit(cmd.argNumber, cmd.arguments);
    else if(command==2)  save(cmd.argNumber, cmd.arguments);
    else if(command==3)  load(cmd.argNumber, cmd.arguments);
    ///Directory commands
    else if(command==4)  cd(cmd.argNumber, cmd.arguments);
    else if(command==5)  ls(cmd.argNumber, cmd.arguments);
    else if(command==6)  mkDir(cmd.argNumber, cmd.arguments);
    else if(command==7)  rm(cmd.argNumber, cmd.arguments);
    else if(command==8)  mv(cmd.argNumber, cmd.arguments);
    else if(command==9)  cp(cmd.argNumber, cmd.arguments);
    ///File editing commands
    else if(command==10) myOpen(cmd.argNumber, cmd.arguments);
    else if(command==11) closef(cmd.argNumber, cmd.arguments);
    else if(command==12) savef(cmd.argNumber, cmd.arguments);
    else if(command==13) myRead(cmd.argNumber, cmd.arguments);
    else if(command==14) readl(cmd.argNumber, cmd.arguments);
    else if(command==15) writel(cmd.argNumber, cmd.arguments);
    else if(command==16) removel(cmd.argNumber, cmd.arguments);

    free(cmd.command);
    for(int i=0; i<cmd.argNumber; i++) free(cmd.arguments[i]);
}
