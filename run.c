#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// this struct is used for multi-threading
struct readArgs {
  char *filename;
  long blockSize;
  long blockCount;
  // the point in the file that the thread should start reading from
  off_t offset;
  int id;
};

// this function was provided by Kamen in the
// final project notion document
unsigned int xorbuf(unsigned int *buffer, int size) {
  unsigned int result = 0;
  for (int i = 0; i < size; i++) {
    result ^= buffer[i];
  }
  return result;
}

// read blockCount number of blocks of size blockSize
// starting at the given offset
// quiet = 1 means do not print anything to stdout
// quiet = 0 means print XOR to stdout
unsigned int readFile(char *filename, long blockSize, long blockCount,
                      off_t offset, int quiet) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  // if offset is 0, then we read from the start of the file
  // and there is no need to call lseek
  if (offset != 0) {
    // from lseek man page:
    // SEEK_SET: "The file offset is set to offset bytes."
    off_t lseekOffset = lseek(fd, offset, SEEK_SET);
    if (lseekOffset == -1) {
      perror("lseek");
      exit(1);
    }
  }

  // allocate space for one block
  unsigned int *buffer = malloc(blockSize);
  if (buffer == NULL) {
    fprintf(stderr, "failed to allocate memory\n");
    exit(1);
  }

  // keep track of the xor after each read
  unsigned int xor_running = 0;

  // track the number of bytes read from the file
  size_t bytesRead;

  for (int i = 0; i < blockCount; i++) {
    bytesRead = read(fd, buffer, blockSize);
    if (bytesRead == -1) {
      perror("read");
      exit(1);
    }
    // the second argument to xorbuff is the number of ints
    // in buffer that we want to calculate the xor of.
    // bytesRead / 4 gives the number of integers that we read in.
    xor_running = xor_running ^ xorbuf(buffer, bytesRead / 4);
  }

  if (!quiet) {
    printf("%08x\n", xor_running);
  }

  free(buffer);
  close(fd);

  return xor_running;
}

// each thread executes readFileThread, which
// passes the values in the readArgs struct to readFile.
// a thread can only be passed a single argument, which
// is why the readArgs struct is needed.
void *readFileThread(void *arg) {
  struct readArgs *args = (struct readArgs *)arg;

  // we cast the XOR to a long because otherwise there's an
  // error when trying to pass it to pthread_exit
  long xor = (long)readFile(args->filename, args->blockSize, args->blockCount,
                            args->offset, 1);

  pthread_exit((void *)xor);
}

// this function implements multi-threaded reads
// if the totalBlockCount is -1, then the entire file is read
// otherwise, totalBlockCount number of blocks are read
int readFileFast(char *filename, long blockSize, long totalBlockCount,
                 int numThreads, int quiet) {
  // the total number of bytes that we want to read from the file
  // note that if a totalBlockCount is passed, then
  // totalFileSize may be less than the size of the entire file
  off_t totalFileSize;

  if (totalBlockCount == -1) {
    // read entire file, not a specific block count
    // call stat to get the size of the file in bytes
    struct stat statbuf;

    int statRet = stat(filename, &statbuf);
    if (statRet == -1) {
      perror("stat");
      exit(1);
    }

    totalFileSize = statbuf.st_size;
  } else {
    // use the given block count
    totalFileSize = totalBlockCount * blockSize;
  }

  int bytesPerInt = 4;

  // if we want to read fewer than one block per thread,
  // then do the read in a single thread
  if (totalFileSize < (numThreads * blockSize)) {
    if (totalBlockCount != -1) {
      // only print this error message if a specific block count was passed
      // to the function
      fprintf(stderr, "block count * block size < num threads * block size\n");
    }
    // determine the number of blocks this single thread needs to read
    // we cast blockSize to a long double because we do not want integer division
    // here. we want to keep the decimal and then ceil it
    long blockCount = (long)ceill(totalFileSize / (long double)blockSize);
    readFile(filename, blockSize, blockCount, 0, quiet);
    exit(1);
  }

  // we need each thread to read a multiple of 4 bytes, but totalFileSize / numThreads
  // might not be a multiple of 4.
  // here we use integer division to divide (totalFileSize / numThreads) by 4, which
  // gets rid of any remainder. we then multiply by 4, giving us a multiple of 4 bytes.
  // this might be superfluous, see comment on realBytesPerThread
  off_t idealBytesPerThread =
      bytesPerInt * (totalFileSize / (numThreads * bytesPerInt));

  // idealBytesPerThread might not be an even multiple of blockSize.
  // we use integer division to ignore any decimal remainder.
  // blockCountPerThread is the number of full blocks that each
  // thread will read. only the last thread's last read can
  // be for an unfull block
  off_t blockCountPerThread = idealBytesPerThread / blockSize;

  // the number of bytes that each thread will read, except the last one.
  // when running for performance, we know that blockSize will be a multiple
  // of 4. therefore, realBytesPerThread will be a multiple of 4.
  off_t realBytesPerThread = blockSize * blockCountPerThread;

  // our code for using pthreads is based on the code provided to us in HW 4
  pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);
  if (threads == NULL) {
    fprintf(stderr, "failed to allocate memory for threads\n");
    exit(1);
  }

  // array of pointers to readArgs structs
  struct readArgs *args[numThreads];

  for (int i = 0; i < numThreads; i++) {
    // offest is the point in the file that this thread should
    // start reading at
    off_t offset = i * realBytesPerThread;

    long blockCount = blockCountPerThread;

    // the last thread will read any extra bytes that could not be
    // evenly distributed to the other threads.
    if (i == numThreads - 1) {
      // the other threads have been assigned a total of offset bytes.
      // we know that totalFileSize will be a multiple of 4 bytes.
      // we also know that offset is a multiple of 4 bytes, since it's
      // a multiple of realBytesPerThread.
      // therefore, bytesLeft will be a multiple of 4.
      off_t bytesLeft = totalFileSize - offset;
      // we cast bytesLeft to a long double because we do not want
      // integer division here. if there's a remainder, we want to round up
      blockCount = (long)ceill((long double)bytesLeft / blockSize);
    }

    args[i] = malloc(sizeof(struct readArgs));
    if (args[i] == NULL) {
      fprintf(stderr, "failed to allocate memory for pointer to thread %d\n",
              i);
      exit(1);
    }

    args[i]->filename = filename;
    args[i]->blockSize = blockSize;
    args[i]->blockCount = blockCount;
    args[i]->offset = offset;
    args[i]->id = i;

    // create a new thread that will run readFileThread(args[i])
    int threadCreateRet =
        pthread_create(&threads[i], NULL, readFileThread, args[i]);
    if (threadCreateRet != 0) {
      fprintf(stderr, "failed create thread %d\n", i);
      exit(1);
    }
  }
  // keep track of the xor after each thread finishes
  long xor = 0;

  for (int i = 0; i < numThreads; i++) {
    long xorThread;
    int threadJoinRet = pthread_join(threads[i], (void **)&xorThread);
    if (threadJoinRet != 0) {
      fprintf(stderr, "failed join thread %d\n", i);
      exit(1);
    }
    xor = xor^xorThread;
  }

  if (quiet == 0) {
    printf("%08lx\n", xor);
  }

  for (int i = 0; i < numThreads; i++) {
    free(args[i]);
  }

  free(threads);

  return 0;
}

// write blockCount number of blocks of size blockSize to a file called filename
int writeFile(char *filename, long blockSize, long blockCount) {
  int fd =
      open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd == -1) {
    perror("open");
  }

  unsigned int *buffer = malloc(blockSize);

  for (int i = 0; i < blockCount; i++) {
    int writeRet = write(fd, buffer, blockSize);
    if (writeRet == -1) {
      perror("write");
      exit(1);
    }
  }

  free(buffer);
  close(fd);

  return 0;
}

// normal read and write:
// ./run <filename> [-r|-w] <block_size> <block_count> [-q]
// fast read entire file:
// ./run <filename> -f <block_size> <num_threads> [-q]
// fast read specific number of blocks:
// ./run <filename> -t <block_size> <num_threads> <block_count>
int main(int argc, char **argv) {
  // check for correct number of arguments
  if (argc < 5 || argc > 6) {
    fprintf(stderr,
            "Incorrect number of arguments. Use the command format below\n");
    fprintf(stderr, "for normal read or write:\n./run <filename> [-r|-w] "
                    "<block_size> <block_count> [-q]\n");
    fprintf(
        stderr,
        "for fast read:\n./run <filename> -f <block_size> <nu_threads> [-q]\n");
    return 1;
  }

  char *filename = argv[1];
  // by default, not quiet
  int quiet = 0;

  // if the mode is -r, -w, or -f (aka not -t)
  // then the 6th argument can only be the quiet flag
  if (argc == 6 && strcmp(argv[2], "-t") != 0) {
    if (strcmp(argv[5], "-q") == 0) {
      quiet = 1;
    } else {
      fprintf(stderr, "unknown flag %s\n", argv[5]);
      return 1;
    }
  }

  if (strcmp(argv[2], "-f") == 0) {
    // fast read entire file
    long blockSize = atol(argv[3]);
    int numThreads = atoi(argv[4]);
    if (blockSize <= 0 || numThreads <= 0) {
      fprintf(stderr, "invalid block size or number of threads\n");
      return 1;
    }
    // we pass -1 as totalBlockCount since we want to read the entire file
    readFileFast(filename, blockSize, -1, numThreads, quiet);
  } else if (strcmp(argv[2], "-t") == 0) {
    // fast read specific block count
    long blockSize = atol(argv[3]);
    int numThreads = atoi(argv[4]);
    long numBlocks = atol(argv[5]);
    if (blockSize <= 0 || numThreads <= 0 || numBlocks <= 0) {
      fprintf(stderr,
              "invalid block size, number of threads, or block count\n");
      return 1;
    }
    // the 1 at the end is to make it quiet
    readFileFast(filename, blockSize, numBlocks, numThreads, 1);
  } else {
    // normal read or write
    long blockSize = atol(argv[3]);
    long blockCount = atol(argv[4]);
    if (blockSize <= 0 || blockCount <= 0) {
      fprintf(stderr, "invalid block size or block count\n");
      return 1;
    }
    if (strcmp(argv[2], "-r") == 0) {
      // the 0 is for offset. read from the beginning
      readFile(filename, blockSize, blockCount, 0, quiet);
    } else if (strcmp(argv[2], "-w") == 0) {
      writeFile(filename, blockSize, blockCount);
    } else {
      fprintf(stderr, "unknown flag %s\n", argv[2]);
      return 1;
    }
  }
  return 0;
}