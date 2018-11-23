#include "../include/shell.h"

bool session;

int getCmdIdx(char* cmd) {
    char* cmds[]={  "man",
                "quit", "save", "load",
                "cd", "ls", "mkdir", "rm", "mv", "cp",
                "open", "close", "sf", "rd", "rl", "wl", "rml" };
    for(int i=0; i<AVAILABLECMDS; i++) if(strcmp(cmd, cmds[i])==0) return i;

    if(strcmp(cmd, "")!=0) printf("%s is not a recognised command\n", cmd);
    return -1;
}

commandStruct parse(char* cmdStr) {
    //Initialize the command
    commandStruct cmd;
    cmd.argNumber = 0;
    cmd.command = malloc(sizeof(char)*MAXCMD);
    memset(cmd.command, '\0', sizeof(char)*MAXCMD);
    //Eliminate the final newline
    char* pos;
    if ((pos=strchr(cmdStr, '\n')) != NULL) *pos = '\0';
    ///The shell only supports one-word commands, at the start of the user input:
    char * token = strtok(cmdStr, " ");
    if(token!=NULL) strcpy(cmd.command, token);
    //Extract the arguments
    token = strtok(NULL, " ");
    while((token!=NULL)&&(cmd.argNumber<=MAXARGS)) {
        //if an argument starts and ends with ', or some other character then it's fine or it's a user typo
        if((token[0]== '\"')&&(token[strlen(token)-1]!= '\"')) {
            //otherwise it's a SPACED ARGUMENT
            char buff[BLOCKSIZE];
            unsigned int buffSize = sizeof(token);
            memset(buff, '\0', BLOCKSIZE);
            strcpy(buff, token);
            strcat(buff, " ");
            token = strtok(NULL, "\"");
            buffSize += sizeof(token);
            if(buffSize<=BLOCKSIZE) {
                strcat(buff, token);
                strcat(buff, "\"");
            } else {
                printf("The spaced argument provided was too long.\n");
                memset(cmd.command, '\0', MAXCMD);
                return cmd;
            };
            cmd.arguments[cmd.argNumber] = malloc(buffSize);
            memset(cmd.arguments[cmd.argNumber], '\0', buffSize);
            strcpy(cmd.arguments[cmd.argNumber], buff);
        } else {
            cmd.arguments[cmd.argNumber] = malloc((strlen(token)));
            strcpy(cmd.arguments[cmd.argNumber], token);
        }
        token = strtok(NULL, " ");
        cmd.argNumber++;
    }
    if (cmd.argNumber>MAXARGS) {
        printf("The maximum number of arguments has been reached.\n");
        memset(cmd.command, '\0', MAXCMD);
    }
    return cmd;
}
