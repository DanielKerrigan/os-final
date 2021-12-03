#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

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

void callRead(long bSize, long bCount){
    char bSizeStr[64];
    char bCountStr[64];

    sprintf(bSizeStr, "%ld", bSize);
    sprintf(bCountStr, "%ld", bCount);

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

int findReasonableBlockCount(long bSize, int output) {
    // start with reading a single block, double it each iteration
    long bCount = 1;

    // for timing
    double start, end;
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

        start = getTime();
        callRead(bSize, bCount);
        end = getTime();

        delta = end-start;

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

int main(int argc, char **argv) {
    char mode = argv[1][1];

    if (mode == 'r') {
      long blockSize = atol(argv[2]);
      findReasonableBlockCount(blockSize, 1);
    } else if (mode == 'b') {

    }

    return 0;
}