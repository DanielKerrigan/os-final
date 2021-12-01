#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>


void callWrite(char* bSize, char* bCount){
    char* arr[64] = {"run", "out1.txt", "-w", bSize, bCount};
    int pid = fork();
    if(pid == 0){
        execvp("./run", arr);
        perror("Last seen error (w)");
    }
    int ret = wait(NULL);
    if(ret == -1){
        perror("wait");
    }
}

void callRead(char* bSize, char* bCount){
     char* arr[64] = {"run", "out1.txt", "-r", bSize, bCount};
    int pid = fork();
    if(pid == 0){
        execvp("./run", arr);
        perror("Last seen error (r)");
    }
    int ret = wait(NULL);
    if(ret == -1){
        perror("wait");
    }
}

int findReasonable(char* bSize){
    int bCount = 1; 
    time_t start, end;
    char bCountStr[64];
    time_t delta;

    do{
        sprintf(bCountStr, "%d", bCount);
        printf("bcountstr: %s\n", bCountStr);
        
        callWrite(bSize, bCountStr);
        
        time(&start);
        callRead(bSize, bCountStr);
        time(&end);
        delta = end-start;

        bCount *= 2;
    }
    while(end-start < 5);

    long fileSize = (bCount/2)*atoi(bSize)/(1 << 20);
    printf("time taken: %ld\n", delta);
    printf("best file size: %ld \n", fileSize);
    printf("MiB/s: %ld\n", fileSize / delta);

    return 0;
}

int main(int argc, char **argv) {
    char* blockSize = argv[1];
    findReasonable(blockSize);
    
    return 0;
}