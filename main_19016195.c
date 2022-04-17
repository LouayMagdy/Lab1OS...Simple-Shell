#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

void onChildExit(int sig){
    int status;
    pid_t id;
    FILE *logs = fopen("/home/louay/CLionProjects/Lab1OS/logsFile.txt", "a");
    if (logs == NULL){
        printf("Error! Could not create a file\n");
        exit(-1);
    }
    while ((id = waitpid(-1, &status, WNOHANG) )> 0){
        printf("Reaped process %d----%d\n$", id, status);
        fprintf(logs, "process %d terminated\n", id);
        fclose(logs);
    }
}



void split(char splitUpon, char *command, char words[2][2000]);
char *setUpEnvironment();
char * searchVar(char * searchedfor);
int getVarIndexUsr(char * var);
int getVarIndexEnVar(char * var);

void shell(char *currentWorkingDir);
short classifyCommand(char *command1stPartition);
void executeCommand(char *firstPartOfCommand, char *secPartOfCommand, char* workingDir);

///////built-in commands
char* cd(char *secCommandPart);
void echo(char *secCommandPart);
void export(char *secCommandPart);

void lsNoArguments();
void lsWithArguments(char * secPartOfCommand);
void copyOrMove(char * frstPartOfCommand, char * secPartOfCommand);
void removeF(char *secPartOfCommand);
void mkdir(char *secPartOfCommand);
void open(char *firstPartOfCommand, char *secPartOfCommand);

char variablesOS[60][10] = {"\0"};
char valuesOS[60][1000] = {"\0"};

int userDefIndex = 0;
char userDefVar[100][1000] = {"\0"};
char userDefVal[100][1000] = {"\0"};

/////main of parent process
int main(int argc,char* argv[], char** envp) {
    ////////////////////////////register child signal//////// not done yet
    signal(SIGCHLD, onChildExit);
    int i = 0;
    for (char **env = envp; *env != 0; env++){
        char res[2][2000];
        split('=',*env, res);
        strcpy(variablesOS[i], res[0]);
        strcpy(valuesOS[i++], res[1]);
//        printf("%s ....  %s\n", variablesOS[i-1], valuesOS[i-1]);
    }

    ///////setup environment
    char *currentWorkingDir = setUpEnvironment();
    //////shell functionality
    shell(currentWorkingDir);
    return 0;
}

char* setUpEnvironment(){ //////set the working dir to the current one
    static char * currWorkDir;//[PATH_MAX] = {' '};
    currWorkDir = getenv("HOME");
    chdir(currWorkDir);
    return currWorkDir;
}
void split(char splitUpon, char *command, char words[2][2000]){
    int i = 0;
    short foundSplitUpon = 0;
    while(command[i] != '\0'){
        if(command[i] == splitUpon){
            if(foundSplitUpon) words[1][i - strlen(words[0]) - 1] = command[i];
            else {
                words[1][i] = '\0';
                foundSplitUpon= 1;
            }
        }
        else{
            if(foundSplitUpon) words[1][i - strlen(words[0]) - 1] = command[i];
            else words[0][i] = command[i];
        }
        i++;
    }
    words[1][i - strlen(words[0])- 1] = '\0';
}
short classifyCommand(char *command1stPartition){ /////evaluateExpressionStep
    if(!strcmp(command1stPartition, "cd")) return 0;
    if(!strcmp(command1stPartition, "echo")) return 1;
    if(!strcmp(command1stPartition, "export")) return 2;
    if(!strcmp(command1stPartition, "exit")) return 4;
    return 3; /////commands may be not found and may be executed
}
short classifyExecutables(char *command1stPartition){
    if(!strcmp(command1stPartition, "ls\n")) return 1; ////no argument ls
    if(!strcmp(command1stPartition,"ls")) return 2;  /////argumented ls
    if(!strcmp(command1stPartition,"cp") || !strcmp(command1stPartition,"mv")) return 3; ////copy & paste or move
    if(!strcmp(command1stPartition,"rm")) return 4;  /////remove
    if(!strcmp(command1stPartition,"mkdir")) return 5; ////make directory
    return 6;
}

void shell(char *currentWorkingDir){ /////shell functionality
  char command[1000];
    do{
        char words[2][2000] = {"\n", "\n"};
        printf("Current Directory : ");
        puts(currentWorkingDir);
        printf("$ ");
        fgets(command, 1001, stdin);
        split(' ',command, words);
        short classific = classifyCommand(words[0]);
        switch (classific) {
            case 0: ////cd
                currentWorkingDir = cd(words[1]);
                break;
            case 1: ///echo
                echo(words[1]);
                break;
            case 2:////export
                export(words[1]);
                break;
            case 3:////executables
                executeCommand(words[0], words[1], currentWorkingDir);
                break;
            case 4:////exit
                exit(1);
            default:;
        }
    } while (strcmp(command, "exit\n") != 0);
}
char* cd(char *secCommandPart){
    static char currWorkDir[PATH_MAX] = "\0";
    int errors = 0;
    if(strcmp(secCommandPart, "..\n") == 0) errors = chdir("..");
    else if(strcmp(secCommandPart, "~\n") == 0 || strcmp(secCommandPart, "\n") == 0)
        errors = chdir(getenv("HOME"));
    else if(secCommandPart[0] == '/') {
        int i = 0;
        while (secCommandPart[i] != '\n') i++;
        secCommandPart[i] = '\0';
        errors = chdir(secCommandPart);
    }
    else{
        int i = 0;
        while (secCommandPart[i] != '\n') i++;
        secCommandPart[i] = '\0';
        errors = chdir(secCommandPart);
        if(errors){
            char NoDollSign[100] = "";
            i = 1;
            while (secCommandPart[i] != '\0'){
                NoDollSign[i-1] = secCommandPart[i];
                i++;
            }
            NoDollSign[i-1] = '\0';
            strcpy(NoDollSign ,searchVar(NoDollSign));
            errors = chdir(NoDollSign);
        }
    }
    if(errors) printf("No such file or directory!!\n");
    getcwd(currWorkDir, sizeof currWorkDir);
    return currWorkDir;
}
void echo(char *secCommandPart){
    if(secCommandPart[0] != '\"') printf("Invalid Format");
    else{
        char result[2000] = "", var[1000] = "";
        int i = 1, counter = 0, ind = 0;
        while(secCommandPart[i] != '\n' && secCommandPart[i] != '\"'){
            if(secCommandPart[i] == '$'){
                i++;
                while (secCommandPart[i + counter] != '\n' && secCommandPart[i + counter] != ' ' &&
                        secCommandPart[i + counter] != '$' && secCommandPart[i + counter] != '\"'){
                    var[counter] = secCommandPart[i + counter];
                    counter++;
                }
                var[counter] = '\0';
                i += counter;
                counter =0 ;
                strcpy(var ,searchVar(var));
                puts(var);
            }
            else{
                printf("%c \n", secCommandPart[i++]);
            }

        }

    }
}
void export(char *secCommandPart){
    char words[2][2000] = {"\n", "\n"};
    split('=', secCommandPart, words);
    if(words[1][0] == '\"'){ ///in case of quotes
        int i = 1;
        while(words[1][i] != '\n') i++;
        if(words[1][i - 1] != '\"'){
            printf("Invalid Format!!\n");
            return;
        }
        else{
            int k;
            char temp[2000];
            for(k = 1; k < i - 1; k++) temp[k - 1] = words[1][k];
            temp[k-1] = '\0';
            k = getVarIndexUsr(words[0]);
            if(k == -1) {
                strcpy(userDefVar[userDefIndex] ,words[0]);
                strcpy(userDefVal[userDefIndex++] , temp);
            }
            else{
                strcpy(userDefVar[k] ,words[0]);
                strcpy(userDefVal[k] , temp);
            }
        }
    }
    else{/////in case of giving a value without quotes
        char minus[2000] = "";
        if(words[1][0] == '-'){
            int i = 1;
            minus[0] = '-';
            while(words[1][i] != '\n'){
                words[1][i-1] = words[1][i];
                i++;
            }
            words[1][i-1] = '\0';
        }
        else words[1][strlen(words[1]) - 1] = '\0';
        int x = getVarIndexUsr(words[1]);
        if(x != -1) strcpy(words[1], strcat(minus, userDefVal[x]));
        if(x == -1) {
            x = getVarIndexEnVar(words[1]);
            if(x != -1) strcpy(words[1], strcat(minus, valuesOS[x]));
            else strcpy(words[1], strcat(minus, words[1]));
        }

        x = getVarIndexUsr(words[0]);
        if(x != -1) {
            strcpy(userDefVar[x],words[0]);
            strcpy(userDefVal[x],words[1]);
        }
        else{
            x = getVarIndexEnVar(words[0]);
            if(x != -1){
                strcpy(variablesOS[x],words[0]);
                strcpy(valuesOS[x],words[1]);
            }
            else{
                strcpy(userDefVar[userDefIndex],words[0]);
                strcpy(userDefVal[userDefIndex],words[1]);
                userDefIndex++;
            }
        }
    }
}

void executeCommand(char *firstPartOfCommand, char *secPartOfCommand, char* workingDir){
    switch (classifyExecutables(firstPartOfCommand)) {
        case 1:
            lsNoArguments();
            break;
        case 2: /// ls with arguments
            lsWithArguments(secPartOfCommand);
            break;
        case 3: /////Copy & Paste or move
            copyOrMove(firstPartOfCommand, secPartOfCommand);
            break;
        case 4:///remove
            removeF(secPartOfCommand);
            break;
        case 5:////make folder
            mkdir(secPartOfCommand);
            break;
        default:////open
            open(firstPartOfCommand, secPartOfCommand);
    }
}
void lsNoArguments(){
    int status;
    pid_t id = fork();
    if(id == 0){//////the child process where the implementation settles
        char* command = "ls";
        char* argument_list[] = {"ls", NULL, NULL};
        status =  execvp(command, argument_list);
        if(status == -1) printf("Task Not Completed\n");
        exit(0);
    }
    else if(id > 0) {
        if(waitpid(id, &status, WIFEXITED(status)))
            printf("Task Completed Successfully\n");
        else printf("There was a problem in executing this command!..please try again\n");
    }
}
void lsWithArguments(char * secPartOfCommand) {
    int status;
    pid_t id = fork();
    if (id == 0) {//////the child process where the implementation settles
        char *command = "ls";
        char argument_list[10][100] = {"  ", "  ", "  ", "   ", "   ", "    ", "   ", "   ", "  ", "  "};
        strcpy(argument_list[0], "ls");
        char temp[1000] = {'\0'};
        secPartOfCommand[strlen(secPartOfCommand) - 1] = '\0';
        int i = 0, j = 0, ind = 1;
        while (secPartOfCommand[i] != '\0' && secPartOfCommand[i] != '\n') {
            j = 0;
            if (secPartOfCommand[i] == '$') {
                for (j = 1; secPartOfCommand[i + j] != ' ' && secPartOfCommand[i + j] != '\0'
                            && secPartOfCommand[i + j] != '\n'; j++) {
                    temp[j - 1] = secPartOfCommand[i + j];
                }
                temp[j++ - 1] = '\0';
//                puts(temp);

                strcpy(temp, searchVar(temp));
                short found = 0;
                for (j = 1; j < 10; j++) {
                    if (strcmp(argument_list[j], temp) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) strcpy(argument_list[ind++], temp);
            }
            if (secPartOfCommand[i] == '-') {
                temp[0] = '-';
                j = 0;
                for (j = 1; secPartOfCommand[i + j] != ' ' && secPartOfCommand[i + j] != '\0'
                            && secPartOfCommand[i + j] != '\n'; j++) {
                    temp[j] = secPartOfCommand[i + j];
                }
                temp[j++] = '\0';
                short found = 0;
                for (int k = 1; k < 10; k++) {
                    if (strcmp(argument_list[k], temp) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) strcpy(argument_list[ind++], temp);
            }
            i++;
        }
        char *argv[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        for (int k = 0; k < ind; k++) argv[k] = argument_list[k];
//        for(int k = 0; k < ind; k++) puts(argv[k]);
        short needMore = 0;
        for(int k = 0; k < ind; k++){
            int e = 0;
            while(argv[k][e] != '\0' && argv[k][e] != '\n'){
                if(argv[k][e] == ' ') {
                    needMore = 1;
                    break;
                }
                e++;
            }
            if(needMore) break;
        }
        if(needMore){
            int h = 0, l = 0, k = 1;
            char tempa[10][100] = {""};
            char word[100] = "";
            for (int z = 0; z < strlen(argument_list[k]); z++) {
                if (argument_list[k][z] != ' ') word[z - l] = argument_list[k][z];
                else if (argument_list[k][z] == ' ') {
                    word[z -l] = '\0';
                    strcpy(tempa[h++], word);
//                puts(word);
                    l = z + 1;
                    strcpy(word, "");
                }
                if(z == (strlen(argument_list[k]) - 1)){
                    word[z - l + 1] = '\0';
                    strcpy(tempa[h++], word);
//                puts(word);
                    l = z + 1;
                    strcpy(word, "");
                }
            }
            for(int z = 1; z < 10; z++) argv[z]= NULL;
            for(int z = 0; z < h; z++) argv[z+1]= tempa[z];
            for(int z = 0; z < h; z++) puts(argv[z]);
        }
        status = execvp(command, argv);
        if (status == -1) printf("Task not completed\n");
        exit(0);
    }
    else if(id > 0){
        do{
            id = waitpid(id, &status, WIFEXITED(status));
        } while (id == 0);
        //        id = wait(&status);
        printf("Task Completed Successfully\n");
        if(!WIFEXITED(status))
            printf("There was a problem in executing this command!..please try again\n");
    }
}

void copyOrMove(char * frstPartOfCommand, char * secPartOfCommand){
    char words[2][2000] = {""};
    split(' ', secPartOfCommand, words);
    words[1][strlen(words[1]) - 1] = '\0';
    pid_t id = fork();
    int status;
    if(id == 0){
        char *argv[3];
        argv[0] = frstPartOfCommand;

        char temp[100] = "";
        strcpy(temp, words[0]);
        if(words[0][0] == '$'){
            int i = 1;
            while(words[0][i] != '\0'){
                temp[i- 1] = words[0][i];
                i++;
            }
            temp[i -1] = '\0';
            strcpy(temp, searchVar(temp));
        }
        argv[1] = temp;

        char temp2[100] = "";
        strcpy(temp2, words[1]);
        if(words[1][0] == '$'){
            int i = 1;
            while(words[1][i] != '\0'){
                temp[i- 1] = words[1][i];
                i++;
            }
            temp[i -1] = '\0';
            strcpy(temp, searchVar(temp));
        }

        argv[2] = words[1];
        status = execvp(frstPartOfCommand, argv);
        perror("failed");
        if(status == -1) printf("Task Not Completed\n");
        exit(0);
    }
    else if(id > 0) {
        do{
            id = waitpid(id, &status, WIFEXITED(status));
        } while (id == 0);
        printf("Task Completed Successfully\n");
        if(!WIFEXITED(status))
            printf("There was a problem in executing this command!..please try again\n");
    }
}
void removeF(char *secPartOfCommand){
    pid_t id = fork();
    int status;
    if(id == 0){
        secPartOfCommand[strlen(secPartOfCommand) - 1] = '\0';
        char *argv[3];
        argv[0] = "rm";
        char temp[100] = "";
        strcpy(temp, secPartOfCommand);
        if(secPartOfCommand[0] == '$'){
            int i = 1;
            while(secPartOfCommand[i] != '\0'){
                temp[i- 1] = secPartOfCommand[i];
                i++;
            }
            temp[i -1] = '\0';
            strcpy(temp, searchVar(temp));
        }
        argv[1] = temp;
        argv[2] = NULL;
        status = execvp("rm", argv);
        if(status == -1) printf("Task Not Completed\n");
        exit(0);
    }
    else if(id > 0) {
        do{
            id = waitpid(id, &status, WIFEXITED(status));
        } while (id == 0);
        //        id = wait(&status);
        printf("Task Completed Successfully\n");
        if(!WIFEXITED(status))
            printf("There was a problem in executing this command!..please try again\n");
    }
}
void mkdir(char *secPartOfCommand){
    pid_t id = fork();
    int status;
    if(id == 0) {
        secPartOfCommand[strlen(secPartOfCommand) - 1] = '\0';
        char *argv[3];
        argv[0] = "mkdir";
        char temp[100] = "";
        strcpy(temp, secPartOfCommand);
        if(secPartOfCommand[0] == '$'){
            int i = 1;
            while(secPartOfCommand[i] != '\0'){
                temp[i- 1] = secPartOfCommand[i];
                i++;
            }
            temp[i -1] = '\0';
            strcpy(temp, searchVar(temp));
        }
        argv[1] = temp;
        argv[2] = NULL;
        status = execvp("mkdir", argv);
        if (status == -1) printf("Task Not Completed\n");
        exit(0);
    }
    else if(id > 0) {
        do{
            id = waitpid(id, &status, WIFEXITED(status));
        } while (id == 0);
        //        id = wait(&status);
        printf("Task Completed Successfully\n");
        if(!WIFEXITED(status))
        printf("There was a problem in executing this command!..please try again\n");
    }
}
void open(char *firstPartOfCommand, char *secPartOfCommand){
    secPartOfCommand[strlen(secPartOfCommand) - 1] = '\0';
    char * x = "&";
    pid_t id = fork();
    int status = 0;
    if(id == 0) {
        char *argv[3];
        argv[0] = firstPartOfCommand;
        if(secPartOfCommand == x) argv[1] = secPartOfCommand;
        else argv[1] = NULL;
        argv[2] = NULL;
        status = execvp(firstPartOfCommand, argv);
        perror("Program Not Found !! or Invalid Command !!");
        if(secPartOfCommand != x) exit(0);
    }
    else if(id > 0) {
        if(strcmp(secPartOfCommand ,x) != 0){
            do{
                id = waitpid(id, &status, WIFEXITED(status));
            } while (id == 0);
        }
//        printf("Task Completed Successfully\n");
        if(!WIFEXITED(status))
            printf("There was a problem in executing this command!..please try again\n");
    } else{
        printf("Invalid command");
    }
}

char * searchVar(char *searchedfor){
    for(int i = 0; i < 57; i++)
        if(!strcmp(searchedfor, variablesOS[i])) return valuesOS[i];
    for(int i = 0; i < 100; i++)
        if(!strcmp(searchedfor, userDefVar[i])) return userDefVal[i];
    return NULL;
}
int getVarIndexEnVar(char var[1000]){
    for(int i = 0; i < 57; i++)
        if(!strcmp(var, variablesOS[i])) return i;
    return -1;
}
int getVarIndexUsr(char var[1000]){
    for(int i = 0; i < 100; i++){
        if(!strcmp(var, userDefVar[i]))
            return i;
    }
    return -1;
}
