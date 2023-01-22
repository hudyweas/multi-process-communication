#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_MSG_SIZE    1024    //max normal message/input size
#define MAX_RESULT_SIZE 4096    //max response message size from other process
#define CLS_SCR         true    //true - clear screen after invalid input

// Author: https://stackoverflow.com/users/179910/jerry-coffin
//         https://stackoverflow.com/questions/29788983/split-char-string-with-multi-character-delimiter-in-c
char *multi_tok(char *input, char *delimiter) {
    static char *string;
    if (input != NULL)
        string = input;

    if (string == NULL)
        return string;

    char *end = strstr(string, delimiter);
    if (end == NULL) {
        char *temp = string;
        string = NULL;
        return temp;
    }

    char *temp = string;

    *end = '\0';
    string = end + strlen(delimiter);
    return temp;
}

const char* findFifoPathFromName(char name[]){
    FILE *plik = fopen(".config", "r");

    if (plik == NULL) {
        printf("[ERROR] Błąd otwarcia pliku!\n");
        return NULL;
    }

    char row[MAX_MSG_SIZE];

    int lengthOfName = strlen(name);

    int findFlag = -1;

    while(fgets(row, MAX_MSG_SIZE, plik)){
        if(strncmp(name, row, lengthOfName) == 0){
            findFlag = 1;
            break;
        }
    }

    fclose(plik);

    if(findFlag == 1){
        const char *res = row+lengthOfName+1;
        return res;
    }else{
        const char *res = "-1";
        return res;
    }
};

char *g_fifoPath;
int g_fifo;

char *i_fifoPath;
int i_fifo;

void deleteFifoConnections(){
    close(g_fifo);
    unlink(g_fifoPath);
    close(i_fifo);
    unlink(i_fifoPath);
    exit(0);
}

int main(int argc, char* argv[]){
    signal(SIGINT, deleteFifoConnections);
    int errorStatus;

    if(argc != 2 ){
        printf("[ERROR] Wrong syntax: ./<programName> <processId>\n");
        return 1;
    }

    char *fifoPath = strdup(findFifoPathFromName(argv[1]));
    g_fifoPath = fifoPath;
    // printf("%s\n", fifoPath);
    if(strcmp("-1", fifoPath) == 0){
        printf("[ERROR] Wrong process id\n");
        return 1;
    }

    errorStatus = mkfifo(fifoPath, 0666);
    if(errorStatus == -1){
        if(errno == 17){
            printf("[ERROR] Process already running or path for FIFO file in usage, aborting... \n");
        }else{
            perror("[ERROR] [creating global connection]");
        }
        return 1;
    }
    int fifo = open(fifoPath, O_RDONLY | O_NONBLOCK);
    g_fifo = fifo;
    if(fifo == -1){
        perror("[ERROR] Error during opening FIFO file, aborting");
        return 1;
    }

    pid_t child_pid = fork();

    if(child_pid == 0){
        char sendResultFifoPath[MAX_MSG_SIZE];
        char command[MAX_MSG_SIZE];

        char buffer[MAX_MSG_SIZE];
        while(1){
            char buffer[MAX_MSG_SIZE];
            int readInfo = 0;
            while(readInfo <= 0){
                readInfo = read(fifo, command, MAX_MSG_SIZE);
                sleep(1);
            }
            read(fifo, sendResultFifoPath, MAX_MSG_SIZE);

            int sendResultFifo = open(sendResultFifoPath, O_WRONLY);
            if (sendResultFifo == -1 ){
                perror("[ERROR] Error during opening send result FIFO file, aborting");
                break;
            }

            char result[MAX_RESULT_SIZE];
            FILE *fp = popen(command, "r");
            printf("[INFO] Executing command...\n");
            while(fgets(buffer, sizeof(buffer), fp) != NULL){
                strcat(result, buffer);
                sleep(1);
            }

            printf("[INFO] Sending response\n");
            write(sendResultFifo, result, MAX_RESULT_SIZE);

            printf("[INFO] Closing connection\n");
            close(sendResultFifo);
            pclose(fp);
        }
    }else{
        char buffer[MAX_MSG_SIZE];
        char userInput[MAX_MSG_SIZE];

        char receiverName[MAX_MSG_SIZE];
        char command[MAX_MSG_SIZE];
        char getResultFifoPath[MAX_MSG_SIZE];
        while(1){
            printf("[INFO] Input syntax: <receiverName> &&& <command> &&& <fifoFilePath>\n");
            printf("[INFO] exit: 'e' or 'exit'\n");
            fgets(userInput, MAX_MSG_SIZE, stdin);

            size_t len = strlen(userInput);
            if (len > 0 && userInput[len - 1] == '\n'){
                userInput[--len] = '\0';
            }

            //exiting and closing global connection
            if(strcmp(userInput, "exit") == 0 || strcmp(userInput, "e") == 0){
                printf("[INFO] Leaving program...\n");
                kill(child_pid, SIGKILL);
                close(fifo);
                unlink(fifoPath);
                break;
            }

            //getting data to variables from input string
            printf("[INFO] Preparing user input data\n");
            char *token = multi_tok(userInput, " &&& ");
            int no_tokens = 0;
            while(token != NULL){
                no_tokens += 1;
                if (no_tokens == 1){
                    strcpy(receiverName, token);
                }else if(no_tokens == 2){
                    strcpy(command, token);
                }else if(no_tokens == 3){
                    strcpy(getResultFifoPath, token);
                }
                token = multi_tok(NULL, " &&& ");
            }

            if(no_tokens != 3){
                if(CLS_SCR){
                    system("clear");
                }
                printf("[ERROR] Invalid data input\n");
                continue;
            }

            printf("[INFO] Provided data: %s %s %s\n", receiverName, command, getResultFifoPath);

            //creating inner connection 
            printf("[INFO] Creating inner connection\n");
            errorStatus = mkfifo(getResultFifoPath, 0666);
            i_fifoPath = getResultFifoPath;
            if(errorStatus == -1){
                perror("[ERROR] [creating inner connection]");
                continue;
            }

            char error[100];
            int resultFifo = open(getResultFifoPath, O_RDONLY | O_NONBLOCK);
            i_fifo = resultFifo;
            if(resultFifo == -1){
                perror("[ERROR] Error during opening FIFO file, aborting");
                continue;
            }

            char *receiverFifoPath = strdup(findFifoPathFromName(receiverName));
            if(receiverFifoPath == "-1"){
                printf("[ERROR] Wrong receiver id\n");
                continue;
            }

            int receiverFifo = open(receiverFifoPath, O_WRONLY);
            if(receiverFifo == -1){
                perror("[ERROR] Error during opening FIFO queue file, aborting");
                continue;
            }
            
            //sending data
            errorStatus = write(receiverFifo, command, MAX_MSG_SIZE);
            if(errorStatus == -1){
                perror("[ERROR] Error during sending data");
            }
            errorStatus = write(receiverFifo, getResultFifoPath, MAX_MSG_SIZE);
            if(errorStatus == -1){
                perror("[ERROR] Error during sending data");
            }

            //receiving data
            char result[MAX_RESULT_SIZE] = { 0 };
            int res = 0;
            while(res <= 0){
                res = read(resultFifo, result, MAX_RESULT_SIZE);
                sleep(1);
            }

            //closing inner connection
            printf("[INFO] Closing inner connection\n");
            close(resultFifo);
            unlink(getResultFifoPath);

            printf("%s\n", result);

            sleep(1);
        }
    }
}