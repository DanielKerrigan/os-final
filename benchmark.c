#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>


double getTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

void callWrite(long bSize, long bCount){
    char bSizeStr[64];
    char bCountStr[64];

    sprintf(bSizeStr, "%ld", bSize);
    sprintf(bCountStr, "%ld", bCount);

    char* arr[64] = {"run", "out1.txt", "-w", bSizeStr, bCountStr};
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

double callRead(long bSize, long bCount){
    char bSizeStr[64];
    char bCountStr[64];

    double start, end;
    double delta;

    sprintf(bSizeStr, "%ld", bSize);
    sprintf(bCountStr, "%ld", bCount);

    char* arr[64] = {"run", "out1.txt", "-r", bSizeStr, bCountStr, "-q"};

    start = getTime();
    int pid = fork();
    if(pid == 0){
        execvp("./run", arr);
        perror("Last seen error (r)");
    }
    int ret = wait(NULL);
    if(ret == -1){
        perror("wait");
    }
    end = getTime();

    return end-start;
}

double callLseek(){
    double start, now;
    int counter = 0;
    int numSec = 5;

    int fd = open("out1.txt", O_RDONLY);
    
    if (fd == -1) {
        perror("open");
        return 1;
    }

    start = getTime();
    now = getTime();
    while(now - start < numSec){
        lseek(fd, 0, SEEK_SET);
        counter++;
        now = getTime();
    }
    
    return counter/(now-start);
}

double callGetpid(){
    double start, now;
    int counter = 0;
    int numSec = 5;
    
    start = getTime();
    now = getTime();
    while(now - start < numSec){
        getpid();
        counter++;
        now = getTime();
    }

    return counter/(now-start);
}

double callGetuid(){
    double start, now;
    int counter = 0;
    int numSec = 5;
    
    start = getTime();
    now = getTime();
    while(now - start < numSec){
        getuid();
        counter++;
        now = getTime();
    }

    return counter/(now-start);
}

double callFstat(){
    double start, now;
    int counter = 0;
    int numSec = 5;
    
    int fd = open("out1.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat statbuf;

    start = getTime();
    now = getTime();

    while(now - start < numSec){
        fstat(fd, &statbuf);
        counter++;
        now = getTime();
    }

    return counter/(now-start);
}

long findReasonableBlockCount(long bSize, int output) {
    // start with reading a single block, double it each iteration
    long bCount = 1;

    // for timing
    double delta;

    long testFileSize = 0;
    long reasonableFileSize = 0;

    // create a big test file at the start
    long testFileBlockSize = 1 << 20; // 1 MiB
    long testFileBlockCount = 4096;
    callWrite(testFileBlockSize, testFileBlockCount);

    do {
        // the current test file is not big enough, create a new one
        reasonableFileSize = bCount * bSize;
        testFileSize = testFileBlockSize * testFileBlockCount;
        if (reasonableFileSize > testFileSize) {
          testFileBlockCount = 10 * (bCount * bSize) / testFileBlockSize;
          callWrite(testFileBlockSize, testFileBlockCount);
        }

        delta =  callRead(bSize, bCount);


        bCount *= 2;
    }
    while(delta < 5);

    bCount /= 2;

    if (output) {
      reasonableFileSize = bCount * bSize;
      double fileSizeMiB = ((double) reasonableFileSize) / (1 << 20);

      printf("reasonable file size: %.2f MiB\n", fileSizeMiB);
      printf("time taken to read: %f seconds\n", delta);
      printf("block size: %ld\n", bSize);
      printf("block count: %ld\n", bCount);
    }

    return bCount;
}

int benchmarkData(long* bSizes, int SIZE){

    printf("bSize,bCount,run,MBspeed,Bspeed\n");
    for(int i = 0; i < SIZE; i++){                              //for each block size 
        long bCount = findReasonableBlockCount(bSizes[i], 0);   //determine block count, don't print, callRead
        for(int j = 0; j < 10; j++){                            //run 10 times
            double timeToRead = callRead(bSizes[i], bCount);   
            long fileSize = bSizes[i] * bCount;
            double bytesPerSec = ((double) fileSize) / timeToRead;  //find speed in bytes/sec
            printf("%ld,%ld,%d,%.2f,%.2f\n", bSizes[i], bCount, j, bytesPerSec/(1 << 20), bytesPerSec);   //write csv entry: bSize,bCount,run#,Mib/s     
        }
    }
    return 0;
}

int systemCalls(){
    printf("call,run,speed\n");
    for(int j = 0; j < 10; j++){           
        double numTimesPerSec = callLseek();   
        printf("lseek,%d,%.2f\n", j, numTimesPerSec);   //write csv entry:run#,Metric            
        numTimesPerSec = callGetpid();   
        printf("getpid,%d,%.2f\n", j, numTimesPerSec);   //write csv entry:run#,Metric 
        numTimesPerSec = callGetuid();   
        printf("getuid,%d,%.2f\n", j, numTimesPerSec);   //write csv entry:run#,Metric 
        numTimesPerSec = callFstat();   
        printf("fstat,%d,%.2f\n", j, numTimesPerSec);   //write csv entry:run#,Metric     
    }
    
    return 0;
}


//.benchmark [-r|-b|-o]
int main(int argc, char **argv) {
    char mode = argv[1][1];

    if (mode == 'r') {
      long blockSize = atol(argv[2]);
      findReasonableBlockCount(blockSize, 1);
    } else if (mode == 'b') {
        long bSizes [6] = {4, 8, 16, 32, 64, 128};
        benchmarkData(bSizes, 6);
    }
    else if (mode == 'o') {
        long bSizes [1] = {1};
        benchmarkData(bSizes, 1);
    }
    else if(mode == 'l'){
        systemCalls();
    }

    return 0;
}