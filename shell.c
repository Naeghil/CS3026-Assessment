#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

extern bool session;

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
    printf("Initializing runtime structures...");
    initStructs();
    printf("Runtime structures initialized.");
    printf("Type in 'man' for details on available commands\n");
}

commandStruct getCommand() {
    char commandString[MAXCMD];
    printf(workingDirPath());
    ///TODO: does this print a new line as well?
    printf(":");
    fgets(commandString, sizeof commandString, stdin);
    return parse(commandString);
}

void execute(commandStruct cmd) {
    int command = getCmdIdx(cmd.command);
    switch (command) {
    case 0: printManual(cmd.argNumber, cmd.arguments);
    ///Session commands
    case 1: quit(cmd.argNumber, cmd.arguments);
        break;
    case 2: save(cmd.argNumber, cmd.arguments);
        break;
    case 3: load(cmd.argNumber, cmd.arguments);
        break;
    ///Directory commands
    case 4: cd(cmd.argNumber, cmd.arguments);
        break;
    case 5: ls(cmd.argNumber, cmd.arguments);
        break;
    case 6: mkDir(cmd.argNumber, cmd.arguments);
        break;
    case 7: rm(cmd.argNumber, cmd.arguments);
        break;
    case 8: mv(cmd.argNumber, cmd.arguments);
        break;
    case 9: cp(cmd.argNumber, cmd.arguments);
        break;
    ///File editing commands
    case 10: myOpen(cmd.argNumber, cmd.arguments);
        break;
    case 11: closef(cmd.argNumber, cmd.arguments);
        break;
    case 12: savef(cmd.argNumber, cmd.arguments);
        break;
    case 13: myRead(cmd.argNumber, cmd.arguments);
        break;
    case 14: readl(cmd.argNumber, cmd.arguments);
        break;
    case 15: writel(cmd.argNumber, cmd.arguments);
        break;
    case 16: removel(cmd.argNumber, cmd.arguments);
        break;
    }

    free(cmd.command);
    for(int i=0; i<cmd.argNumber; i++) free(cmd.arguments[i]);
    free(cmd.arguments);
}
