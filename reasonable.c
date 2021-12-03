#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

double getTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

void callWrite(int bSize, int bCount){
    char bSizeStr[64];
    char bCountStr[64];

    sprintf(bSizeStr, "%d", bSize);
    sprintf(bCountStr, "%d", bCount);

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

void callRead(int bSize, int bCount){
    char bSizeStr[64];
    char bCountStr[64];

    sprintf(bSizeStr, "%d", bSize);
    sprintf(bCountStr, "%d", bCount);

    char* arr[64] = {"run", "out1.txt", "-r", bSizeStr, bCountStr};
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

int findReasonable(int bSize) {
    // start with reading a single block, double it each iteration
    int bCount = 1;

    // for timing
    double start, end;
    double delta;

    long testFileSize = 0;
    long reasonableFileSize = 0;

    // create a big test file at the start
    int testFileBlockSize = 1 << 20; // 1 MiB
    int testFileBlockCount = 96;
    callWrite(testFileBlockSize, testFileBlockCount);

    do {
        // the current test file is not big enough, create a new one
        reasonableFileSize = bCount * bSize;
        testFileSize = testFileBlockSize * testFileBlockCount;
        if (reasonableFileSize > testFileSize) {
          testFileBlockCount = 10 * (bCount * bSize) / testFileBlockSize;
          callWrite(testFileBlockSize, testFileBlockCount);
        }

        start = getTime();
        callRead(bSize, bCount);
        end = getTime();

        delta = end-start;

        bCount *= 2;
    }
    while(delta < 10);

    bCount /= 2;

    reasonableFileSize = bCount * bSize;
    double fileSizeMiB = ((double) reasonableFileSize) / (1 << 20);

    printf("reasonable file size: %.2f MiB\n", fileSizeMiB);
    printf("time taken to read: %f seconds\n", delta);
    printf("block size: %d\n", bSize);
    printf("block count: %d\n", bCount);

    return 0;
}

int main(int argc, char **argv) {
    int blockSize = atoi(argv[1]);
    findReasonable(blockSize);

    return 0;
}