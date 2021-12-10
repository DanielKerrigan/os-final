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

enum cache { EMPTY, ADD, NOP };

double callRead(long bSize, long bCount, char* filename) {
  char bSizeStr[64];
  char bCountStr[64];

  double start, end;
  double delta;

  sprintf(bSizeStr, "%ld", bSize);
  sprintf(bCountStr, "%ld", bCount);

  char *arr[64] = {"run", filename, "-r", bSizeStr, bCountStr, "-q"};

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

double callLseek(int numCalls, char* filename) {
  double start, end;

  int fd = open(filename, O_RDONLY);

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

  start = getTime();
  for(int i = 0; i < numCalls; i++){
    getuid();
  }
  end = getTime();

  return numCalls / (end - start);
}

double callFstat(int numCalls, char* filename) {
  double start, end;

  int fd = open(filename, O_RDONLY);
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

long findReasonableBlockCount(long bSize, char *filename, off_t testFileSize, enum cache action, int output) {
  // start with reading a single block, double it each iteration
  long bCount = 1;

  // for timing
  double delta;

  long reasonableFileSize;

  do {
    reasonableFileSize = bCount * bSize;

    if (reasonableFileSize > testFileSize) {
      fprintf(stderr, "test file size is too small\n");
      exit(1);
    }

    if (action == EMPTY) {
      clearCache();
    } else if (action == ADD) {
      callRead(bSize, bCount, filename);
    }

    delta = callRead(bSize, bCount, filename);

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

int benchmarkData(long *bSizes, int SIZE, enum cache action, char* filename, off_t testFileSize) {

  printf("bSize,bCount,run,MBspeed,Bspeed,seconds\n");
  // for each block size
  for (int i = 0; i < SIZE; i++) {
    // determine block count, don't print, callRead
    long bCount = findReasonableBlockCount(bSizes[i], filename, testFileSize, action, 0);
    // run 10 times
    if(action == ADD){
      callRead(bSizes[i], bCount, filename); //run once to be sure the file is cached
    }
    for (int j = 0; j < 10; j++) {
      if(action == EMPTY){
        clearCache();
      }
      double timeToRead = callRead(bSizes[i], bCount, filename);
      long fileSize = bSizes[i] * bCount;
      // find speed in bytes/sec
      double bytesPerSec = ((double)fileSize) / timeToRead;
      // write csv entry: bSize,bCount,run#,Mib/s
      printf("%ld,%ld,%d,%.2f,%.2f,%.2f\n", bSizes[i], bCount, j,
             bytesPerSec / (1 << 20), bytesPerSec, timeToRead);
    }
  }
  return 0;
}

int systemCalls(char* filename) {
  int numCalls = 25000000;
  printf("call,run,speed\n");
  for (int j = 0; j < 10; j++) {
    double numTimesPerSec = callLseek(numCalls, filename);
    // write csv entry:run#,Metric
    printf("lseek,%d,%.2f\n", j, numTimesPerSec);

    numTimesPerSec = callGetpid(numCalls);
    // write csv entry:run#,Metric
    printf("getpid,%d,%.2f\n", j, numTimesPerSec);

    numTimesPerSec = callGetuid(numCalls);
    // write csv entry:run#,Metric
    printf("getuid,%d,%.2f\n", j, numTimesPerSec);

    numTimesPerSec = callFstat(numCalls, filename);
    // write csv entry:run#,Metric
    printf("fstat,%d,%.2f\n", j, numTimesPerSec);

    // numTimesPerSec = callIncr(numCalls);
    // // write csv entry:run#,Metric
    // printf("increment,%d,%.2f\n", j, numTimesPerSec);
  }

  return 0;
}

//.benchmark [-r|-b|-o|-l|-c] <filename> [block_size]
int main(int argc, char **argv) {
  char mode = argv[1][1];
  char *filename = argv[2];


  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;
  }
  struct stat statbuf;
  fstat(fd, &statbuf);
  off_t testFileSize = statbuf.st_size;

  // clearCache();
  // double t = callRead(1024, 2752674);
  // printf("time to read: %f\n", t);
  // t = callRead(1024, 2752674);
  // printf("time to read: %f\n", t);

  if (mode == 'r') {
    // find block size for one block count
    long blockSize = atol(argv[3]);
    findReasonableBlockCount(blockSize, filename, testFileSize, NOP, 1);
  } else if (mode == 'b') {
    // output csv with caching
    long bSizes[1] = {1024};
    benchmarkData(bSizes, 1, ADD, filename, testFileSize);
  } else if (mode == 'o') {
    // output csv with one bytes
    long bSizes[1] = {1};
    benchmarkData(bSizes, 1, ADD, filename, testFileSize);
  } else if (mode == 'l') {
    // output csv for system calls
    systemCalls(filename);
  } else if (mode == 'c') {
    // output csv without caching
    long bSizes[1] = {1024};
    benchmarkData(bSizes, 1, EMPTY, filename, testFileSize);
  }

  return 0;
}