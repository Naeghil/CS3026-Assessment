#include "../include/shell.h"

bool session;

int getCmdIdx(char* cmd) {
char* cmds[]={  "man",
                "quit", "save", "load",
                "cd", "ls", "mkdir", "rm", "mv", "cp",
                "open", "close", "sf", "rd", "rl", "wl", "rml" };
    for(int i=0; i<AVAILABLECMDS; i++) if(strcmp(cmd, cmds[i])==0) return i;

    if(strcmp(cmd, "")!=0) printf("%s is not a recognised command", cmd);
    return -1;
}

commandStruct parse(char* cmdStr) {
    commandStruct cmd;  initCommand(&cmd);
    ///The shell only supports one-word commands, at the start of the user input:
    char * token = strtok(cmdStr, " ");
    if(token!=NULL) strcpy(cmd.command, token);

    token = strtok(NULL, " ");
    while((token!=NULL)&&(cmd.argNumber<=MAXARGS)) {
        cmd.arguments[cmd.argNumber] = malloc(sizeof(char)*(strlen(token)));
        strcpy(cmd.arguments[cmd.argNumber], token);
        ///TODO: check spaced arguments and implement it with malloc
        ///This won't work with paths including spaced stuff
        if ((token[0] == '\'')&&(token[strlen(token)-2] != '\'')) spacedArg(&cmd, cmd.argNumber);
        token = strtok(NULL, " ");
        cmd.argNumber++;
    }

    if (cmd.argNumber>MAXARGS) {
        printf("the maximum number of arguments has been reached");
        memset(cmd.command, '\0', sizeof(char)*MAXCMD);
    }
    return cmd;
}

void spacedArg(commandStruct * cmd, int idx) {
    char * token="";
    do {
        token = strtok(NULL, " ");
        strcat(cmd->arguments[idx], " ");
        strcat(cmd->arguments[idx], token);
    } while (token[strlen(token)-2]!='\'');
}

void initCommand(commandStruct* cmd) {
    cmd->argNumber = 0;
    cmd->command = malloc(sizeof(char)*MAXCMD);
    memset(cmd->command, '\0', sizeof(char)*MAXCMD);
    cmd->arguments = malloc(sizeof(char*)*MAXARGS);
}
