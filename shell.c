#include <stdio.h>
#include <stdlib.h>

#include "include/shell.h"

extern bool session;
dirNode* workingDir;
dirNode* directoryHierarchy;

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
    char commandString[(MAXARGS+2)*MAXCMD];
    memset(commandString, '\0', (MAXARGS+2)*MAXCMD);
    printf(workingDirPath());
    fgets(commandString, (MAXARGS+2)*MAXCMD, stdin);
    commandStruct cmd = parse(commandString);
    return cmd;
}
