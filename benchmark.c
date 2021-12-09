#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


double getTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

void callWrite(long bSize, long bCount) {
  char bSizeStr[64];
  char bCountStr[64];

  sprintf(bSizeStr, "%ld", bSize);
  sprintf(bCountStr, "%ld", bCount);

  char *arr[64] = {"run", "out1.txt", "-w", bSizeStr, bCountStr};
  int pid = fork();
  if (pid == 0) {
    execvp("./run", arr);
    perror("Last seen error (w)");
  }
  int ret = wait(NULL);
  if (ret == -1) {
    perror("wait");
  }
}

double callRead(long bSize, long bCount) {
  char bSizeStr[64];
  char bCountStr[64];

  double start, end;
  double delta;

  sprintf(bSizeStr, "%ld", bSize);
  sprintf(bCountStr, "%ld", bCount);

  char *arr[64] = {"run", "ubuntu.iso", "-r", bSizeStr, bCountStr, "-q"};

  start = getTime();
  int pid = fork();
  if (pid == 0) {
    execvp("./run", arr);
    perror("Last seen error (r)");
  }
  int ret = wait(NULL);
  if (ret == -1) {
    perror("wait");
  }
  end = getTime();

  return end - start;
}


void clearCache(){
   char *arr[64] = {"purge"};

  int pid = fork();
  if (pid == 0) {
    execvp(arr[0], arr);
    perror("clear cache error ");
  }
  int ret = wait(NULL);
  if (ret == -1) {
    perror("wait");
  }

}



double callLseek(int numCalls) {
  double start, end;


  int fd = open("out1.txt", O_RDONLY);

  if (fd == -1) {
    perror("open");
    return 1;
  }

  start = getTime();
  for(int i = 0; i < numCalls; i++){
    lseek(fd, 0, SEEK_SET);
  }
  end = getTime();

  return numCalls / (end - start);
}

double callGetpid(int numCalls) {
  double start, end;

  int fd = open("out1.txt", O_RDONLY);

  if (fd == -1) {
    perror("open");
    return 1;
  }

  start = getTime();
  for(int i = 0; i < numCalls; i++){
    getpid();
  }
  end = getTime();

  return numCalls / (end - start);
}

double callIncr(int numCalls) {
  double start, end;
  int j = 0;

  start = getTime();
  for(int i = 0; i < numCalls; i++){
    j++;
  }
  end = getTime();

  return numCalls / (end - start);
}

double callGetuid(int numCalls) {
  double start, end;

  int fd = open("out1.txt", O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  start = getTime();
  for(int i = 0; i < numCalls; i++){
    getuid();
  }
  end = getTime();

  return numCalls / (end - start);
}

double callFstat(int numCalls) {
  double start, end;

  int fd = open("out1.txt", O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;
  }
  struct stat statbuf;

  start = getTime();
  for(int i = 0; i < numCalls; i++){
    fstat(fd, &statbuf);
  }
  end = getTime();

  return numCalls / (end - start);
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

    delta = callRead(bSize, bCount);

    bCount *= 2;
  } while (delta < 5);

  bCount /= 2;

  if (output) {
    reasonableFileSize = bCount * bSize;
    double fileSizeMiB = ((double)reasonableFileSize) / (1 << 20);

    printf("reasonable file size: %.2f MiB\n", fileSizeMiB);
    printf("time taken to read: %f seconds\n", delta);
    printf("block size: %ld\n", bSize);
    printf("block count: %ld\n", bCount);
  }

  return bCount;
}

int benchmarkData(long *bSizes, int SIZE, int clearBool) {

  printf("bSize,bCount,run,MBspeed,Bspeed\n");
  // for each block size
  for (int i = 0; i < SIZE; i++) {
    // determine block count, don't print, callRead
    long bCount = findReasonableBlockCount(bSizes[i], 0);
    // run 10 times
    if(clearBool == 0){
      callRead(bSizes[i], bCount); //run once to be sure the file is cached
    }
    for (int j = 0; j < 10; j++) {
      if(clearBool == 1){
        clearCache();
      }
      double timeToRead = callRead(bSizes[i], bCount);
      long fileSize = bSizes[i] * bCount;
      // find speed in bytes/sec
      double bytesPerSec = ((double)fileSize) / timeToRead;
      // write csv entry: bSize,bCount,run#,Mib/s
      printf("%ld,%ld,%d,%.2f,%.2f\n", bSizes[i], bCount, j,
             bytesPerSec / (1 << 20), bytesPerSec);
    }
  }
  return 0;
}

int systemCalls() {
  int numCalls = 25000000;
  printf("call,run,speed\n");
  for (int j = 0; j < 10; j++) {
    double numTimesPerSec = callLseek(numCalls);
    // write csv entry:run#,Metric
    printf("lseek,%d,%.2f\n", j, numTimesPerSec);

    numTimesPerSec = callGetpid(numCalls);
    // write csv entry:run#,Metric
    printf("getpid,%d,%.2f\n", j, numTimesPerSec);

    numTimesPerSec = callGetuid(numCalls);
    // write csv entry:run#,Metric
    printf("getuid,%d,%.2f\n", j, numTimesPerSec);

    numTimesPerSec = callFstat(numCalls);
    // write csv entry:run#,Metric
    printf("fstat,%d,%.2f\n", j, numTimesPerSec);

    // numTimesPerSec = callIncr(numCalls);
    // // write csv entry:run#,Metric
    // printf("increment,%d,%.2f\n", j, numTimesPerSec);
  }

  return 0;
}

//.benchmark [-r|-b|-o|-l|-c]
int main(int argc, char **argv) {
  // char mode = argv[1][1];
  clearCache();
  double t = callRead(1024, 2752674);
  printf("time to read: %f\n", t);
  t = callRead(1024, 2752674);
  printf("time to read: %f\n", t);

  // if (mode == 'r') {
  //   long blockSize = atol(argv[2]);
  //   findReasonableBlockCount(blockSize, 1);
  // } else if (mode == 'b') {
  //   long bSizes[1] = {1024};
  //   benchmarkData(bSizes, 1, 0);
  // } else if (mode == 'o') {
  //   long bSizes[1] = {1};
  //   benchmarkData(bSizes, 1, 0);
  // } else if (mode == 'l') {
  //   systemCalls();
  // } else if (mode == 'c') {
  //   long bSizes[1] = {1024};
  //   benchmarkData(bSizes, 1, 1);
  // }

  return 0;
}