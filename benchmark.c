#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// EMPTY means clear the cache
// ADD means do extra read to cache data
// NOP means do nothing
enum cache { EMPTY, ADD, NOP };

// this function is based on code given in HW 4
// return the wall time in seconds (with decimal)
double getTime() {
  struct timeval tv;
  int timeOfDayRet = gettimeofday(&tv, NULL);
  if (timeOfDayRet == -1) {
    perror("gettimeofday");
    exit(1);
  }
  return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

// fork and exec ./run for reading bCount blocks of size bSize
// with a single thread
double callRead(long bSize, long bCount, char *filename) {
  // we need to turn the block size and block count into strings
  char bSizeStr[64];
  char bCountStr[64];

  double start, end;

  // turn block size into a string
  int bSizeStrRet = sprintf(bSizeStr, "%ld", bSize);
  if (bSizeStrRet < 0) {
    fprintf(stderr, "error converting bSize to string\n");
    exit(1);
  }

  // turn block count into a string
  int bCountStrRet = sprintf(bCountStr, "%ld", bCount);
  if (bCountStrRet < 0) {
    fprintf(stderr, "error converting bCount to string\n");
    exit(1);
  }

  // create the argument array
  char *arr[64] = {"run", filename, "-r", bSizeStr, bCountStr, "-q"};

  // get starting time before fork
  start = getTime();
  int pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  }
  if (pid == 0) {
    // in the child process, exec the command
    execvp("./run", arr);
    // if exec returns, then there was an error
    perror("execvp");
    exit(1);
  }

  int ret = wait(NULL);
  if (ret == -1) {
    perror("wait");
    exit(1);
  }
  // get the ending time after the child finishes
  end = getTime();

  return end - start;
}

// fork and exec ./run for fast read with a specific block count
double callFast(char *filename, long blockSize, long blockCount,
                int numThreads) {
  // we need to turn the block size, block count, and num threads into strings
  char bSizeStr[64];
  char bCountStr[64];
  char nThreadsStr[64];

  double start, end;

  int bSizeStrRet = sprintf(bSizeStr, "%ld", blockSize);
  if (bSizeStrRet < 0) {
    fprintf(stderr, "error converting blockSize to string\n");
    exit(1);
  }

  int bCountStrRet = sprintf(bCountStr, "%ld", blockCount);
  if (bCountStrRet < 0) {
    fprintf(stderr, "error converting blockCount to string\n");
    exit(1);
  }

  int nThreadsStrRet = sprintf(nThreadsStr, "%d", numThreads);
  if (nThreadsStrRet < 0) {
    fprintf(stderr, "error converting numThreads to string\n");
    exit(1);
  }

  // ./run <filename> -t <block_size> <num_threads> <block_count>
  char *arr[64] = {"run", filename, "-t", bSizeStr, nThreadsStr, bCountStr};

  // get starting time before fork
  start = getTime();
  int pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  }
  if (pid == 0) {
    execvp("./run", arr);
    perror("execvp");
    exit(1);
  }

  int ret = wait(NULL);
  if (ret == -1) {
    perror("wait");
    exit(1);
  }
  // get end time after waiting for exec to exit
  end = getTime();

  return end - start;
}

// fork and exec the clear cache script
void clearCache() {
  char *arr[64] = {"./clear-cache-linux.sh"};

  int pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  }
  if (pid == 0) {
    execvp(arr[0], arr);
    perror("clear cache error");
    exit(1);
  }

  int ret = wait(NULL);
  if (ret == -1) {
    perror("wait");
    exit(1);
  }
}

// fork and exec the clear cache command for MacOS
void clearCacheMac() {
  char *arr[64] = {"purge"};

  int pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  }
  if (pid == 0) {
    execvp(arr[0], arr);
    perror("clear cache error");
    exit(1);
  }

  int ret = wait(NULL);
  if (ret == -1) {
    perror("wait");
    exit(1);
  }
}

// measure how many times lseek can be called per second
double callLseek(int numCalls, char *filename) {
  double start, end;

  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  start = getTime();
  for (int i = 0; i < numCalls; i++) {
    off_t lseekOffset = lseek(fd, 0, SEEK_SET);
    if (lseekOffset == -1) {
      perror("lseek");
      exit(1);
    }
  }
  end = getTime();

  // lseek was called numCalls number of times in (end - start) seconds
  return numCalls / (end - start);
}

// measure how many times getpid can be called per second
double callGetpid(int numCalls) {
  double start, end;

  start = getTime();
  for (int i = 0; i < numCalls; i++) {
    getpid();
  }
  end = getTime();

  return numCalls / (end - start);
}

// measure how many times getuid can be called per second
double callGetuid(int numCalls) {
  double start, end;

  start = getTime();
  for (int i = 0; i < numCalls; i++) {
    getuid();
  }
  end = getTime();

  return numCalls / (end - start);
}

// measure how many times fstat can be called per second
double callFstat(int numCalls, char *filename) {
  double start, end;

  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }
  struct stat statbuf;

  start = getTime();
  for (int i = 0; i < numCalls; i++) {
    int statRet = fstat(fd, &statbuf);
    if (statRet == -1) {
      perror("stat");
      exit(1);
    }
  }
  end = getTime();

  return numCalls / (end - start);
}

// return the number of blocks of size bSize that can be read
// in a reasonable amount of time (> 5 seconds)
long findReasonableBlockCount(long bSize, char *filename, off_t testFileSize,
                              enum cache action, int output) {
  // start with reading a single block, double it each iteration
  long bCount = 1;

  // for timing
  double delta;

  long reasonableFileSize;

  do {
    // make sure that the current block count isnt too big for the given file
    reasonableFileSize = bCount * bSize;

    if (reasonableFileSize > testFileSize) {
      fprintf(stderr, "test file size is too small\n");
      exit(1);
    }

    // handle the cache
    if (action == EMPTY) {
      clearCache();
    } else if (action == ADD) {
      callRead(bSize, bCount, filename);
    }

    // time how long it takes to read the current block count
    delta = callRead(bSize, bCount, filename);

    // double the block count for the next iteration
    bCount *= 2;
  } while (delta < 5); // loop until read took more than 5s

  // we double the block count at the end of the loop body
  // bCount / 2 is the last bCount that was read
  bCount /= 2;

  if (output) {
    reasonableFileSize = bCount * bSize;
    double fileSizeMiB = ((double)reasonableFileSize) / (1L << 20);

    printf("reasonable file size: %.2f MiB\n", fileSizeMiB);
    printf("time taken to read: %f seconds\n", delta);
    printf("block size: %ld\n", bSize);
    printf("block count: %ld\n", bCount);
  }

  return bCount;
}

long findReasonableBlockCountFast(long bSize, int numThreads, char *filename,
                                  off_t testFileSize, enum cache action) {
  // block count must be >= num threads
  long bCount = numThreads;

  // lower threshold for cached threaded version
  double threshold = action == ADD ? 2.5 : 5.0;

  // for cached reads, don't read more than 12 GiB so that
  // we will be able to fit it all in 16 GiB of RAM.
  long maxFileSize = 12 * (1L << 30);

  // for timing
  double delta;

  long reasonableFileSize;

  do {
    reasonableFileSize = bCount * bSize;

    // if the reads are cached, then we cap the maximum
    // file size at 12 GiB.
    if (reasonableFileSize > maxFileSize) {
      long maxBlockCount = maxFileSize / bSize;
      return maxBlockCount;
    }

    if (reasonableFileSize > testFileSize) {
      fprintf(stderr, "test file size is too small\n");
      exit(1);
    }

    // handle the cache
    if (action == EMPTY) {
      clearCache();
    } else if (action == ADD) {
      callFast(filename, bSize, bCount, numThreads);
    }
    // time the read
    delta = callFast(filename, bSize, bCount, numThreads);
    // double the block count for the next iteration
    bCount *= 2;
  } while (delta < threshold); // loop until read takes longer than threshold

  // we double the block count at the end of the loop body
  // bCount / 2 is the last bCount that was read
  bCount /= 2;

  return bCount;
}

// benchmark single-threaded reads
// SIZE is the number of block sizes in the bSizes array
void benchmarkData(long *bSizes, int SIZE, enum cache action, char *filename,
                   off_t testFileSize) {

  printf("cached,bSize,bCount,run,MBspeed,Bspeed,seconds\n");
  // for each block size
  for (int i = 0; i < SIZE; i++) {
    // determine block count, don't print, callRead
    long bCount =
        findReasonableBlockCount(bSizes[i], filename, testFileSize, action, 0);

    if (action == ADD) {
      // run once to be sure the file is cached
      callRead(bSizes[i], bCount, filename);
    }

    // run 10 times
    for (int j = 0; j < 10; j++) {
      if (action == EMPTY) {
        clearCache();
      }
      double timeToRead = callRead(bSizes[i], bCount, filename);
      long fileSize = bSizes[i] * bCount;
      // find speed in bytes/sec
      double bytesPerSec = ((double)fileSize) / timeToRead;
      // write csv entry: cache,bSize,bCount,run#,Mib/s, B/s, total seconds
      printf("%d,%ld,%ld,%d,%.2f,%.2f,%.2f\n", action, bSizes[i], bCount, j,
             bytesPerSec / (1L << 20), bytesPerSec, timeToRead);
    }
  }
}

// benchmark multi-threaded reads
void measureFast(long *blockSizes, int blockSizesLEN, int *numThreads,
                 int numThreadsLEN, enum cache action, char *filename,
                 off_t testFileSize) {

  printf("blockSize,blockCount,numThreads,cached,run,MBspeed,Bspeed,seconds\n");
  for (int nt = 0; nt < numThreadsLEN; nt++) {
    for (int bs = 0; bs < blockSizesLEN; bs++) {
      // determine block count, don't print, callRead
      long bCount = findReasonableBlockCountFast(
          blockSizes[bs], numThreads[nt], filename, testFileSize, action);

      if (action == ADD) {
        // run once to be sure the file is cached
        callFast(filename, blockSizes[bs], bCount, numThreads[nt]);
      }

      // run 5 times
      for (int j = 0; j < 5; j++) {
        if (action == EMPTY) {
          clearCache();
        }
        double timeToRead =
            callFast(filename, blockSizes[bs], bCount, numThreads[nt]);
        // find speed in bytes/sec
        off_t totalReadSize = blockSizes[bs] * bCount;
        double bytesPerSec = ((double)totalReadSize) / timeToRead;
        // write csv entry:
        //blockSize,blockCount,numThreads,cache,run#,MiB/s,B/s,total seconds
        printf("%ld,%ld,%d,%d,%d,%.2f,%.2f,%.2f\n", blockSizes[bs], bCount,
               numThreads[nt], action, j, bytesPerSec / (1L << 20), bytesPerSec,
               timeToRead);
      }
    }
  }
}

// benchmark the system calls
void systemCalls(char *filename) {
  // number of times to call each system call
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
  }
}

#define numBlockSizes 21
#define numBlockSizesFast 13
#define numNumThreads 6

// output instructions for running this program to stderr
void printUsage() {
  fprintf(stderr, "./benchmark option <filename> [<block_size>]\n");
  fprintf(stderr, "where option is one "
                  "of:\n--reasonable\n--perf_cache\n--perf_no_cache\n--perf_"
                  "cache_threads\n--perf_no_cache_threads\n--sys_calls\n");
}

//.benchmark
//[--reasonable|--perf_cache|--perf_no_cache|--perf_cache_threads|--perf_no_cache_threads|--sys_calls]
//<filename> [block_size]
int main(int argc, char **argv) {
  // check number of arguments
  if (argc < 3 || argc > 4) {
    fprintf(stderr,
            "Wrong number of arguments. Use the command format below\n");
    printUsage();
    return 1;
  }

  char *filename = argv[2];

  // open the file and get its size in bytes

  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;
  }
  struct stat statbuf;
  int statRet = fstat(fd, &statbuf);
  if (statRet == -1) {
    perror("stat");
    return 1;
  }

  off_t testFileSize = statbuf.st_size;

  if (strcmp(argv[1], "--reasonable") == 0) {
    // find reasonable block size for one block count
    if (argc != 4) {
      fprintf(stderr,
              "Wrong number of arguments. Use the command format below\n");
      fprintf(stderr, "./benchmark --reasonable filename block_size\n");
      return 1;
    }
    long blockSize = atol(argv[3]);
    findReasonableBlockCount(blockSize, filename, testFileSize, NOP, 1);
  } else if (strcmp(argv[1], "--sys_calls") == 0) {
    // output csv for system calls
    systemCalls(filename);
  } else if (strcmp(argv[1], "--perf_cache") == 0) {
    // output csv with caching, single thread
    long bSizes[numBlockSizes] = {};
    // 1B to 1MiB
    for (int i = 0; i < numBlockSizes; i++) {
      bSizes[i] = 1L << i;
    }
    benchmarkData(bSizes, numBlockSizes, ADD, filename, testFileSize);
  } else if (strcmp(argv[1], "--perf_no_cache") == 0) {
    // output csv without caching, single thread
    long bSizes[numBlockSizes] = {};
    // 1B to 1MiB
    for (int i = 0; i < numBlockSizes; i++) {
      bSizes[i] = 1L << i;
    }
    benchmarkData(bSizes, numBlockSizes, EMPTY, filename, testFileSize);
  } else if (strcmp(argv[1], "--perf_cache_threads") == 0) {
    // output csv with caching & threads
    long bSizes[numBlockSizesFast] = {};
    int nThreads[numNumThreads] = {};
    // start at 64B
    for (int i = 0; i < numBlockSizesFast; i++) {
      bSizes[i] = 1L << (i + 6);
    }
    // 1, 2, 4, 8, 16, 32
    for (int i = 0; i < numNumThreads; i++) {
      nThreads[i] = 1L << i;
    }
    measureFast(bSizes, numBlockSizesFast, nThreads, numNumThreads, ADD,
                filename, testFileSize);
  } else if (strcmp(argv[1], "--perf_no_cache_threads") == 0) {
    // output csv without caching & threads
    long bSizes[numBlockSizesFast] = {};
    int nThreads[numNumThreads] = {};
    // start at 64B
    for (int i = 0; i < numBlockSizesFast; i++) {
      bSizes[i] = 1L << (i + 6);
    }
    // 1, 2, 4, 8, 16, 32
    for (int i = 0; i < numNumThreads; i++) {
      nThreads[i] = 1L << i;
    }
    measureFast(bSizes, numBlockSizesFast, nThreads, numNumThreads, EMPTY,
                filename, testFileSize);
  } else {
    fprintf(stderr, "Unknown option %s. Use options below\n", argv[1]);
    printUsage();
    return 1;
  }
  return 0;
}
