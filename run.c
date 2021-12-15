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

struct readArgs {
  char *filename;
  long blockSize;
  long blockCount;
  off_t offset;
  int id;
};

unsigned int xorbuf(unsigned int *buffer, int size) {
  unsigned int result = 0;
  for (int i = 0; i < size; i++) {
    result ^= buffer[i];
  }
  return result;
}

unsigned int readFile(char *filename, long blockSize, long blockCount,
                      off_t offset, int quiet) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  if (offset != 0) {
    off_t lseekOffset = lseek(fd, offset, SEEK_SET);
    if (lseekOffset == -1) {
      perror("lseek");
      exit(1);
    }
  }

  unsigned int *buffer = malloc(blockSize);
  if (buffer == NULL) {
    fprintf(stderr, "failed to allocate memory\n");
    exit(1);
  }

  unsigned int xor_running = 0;

  size_t bytesRead;

  for (int i = 0; i < blockCount; i++) {
    bytesRead = read(fd, buffer, blockSize);
    if (bytesRead == -1) {
      perror("read");
      exit(1);
    }
    xor_running = xor_running ^ xorbuf(buffer, bytesRead / 4);
  }

  if (!quiet) {
    printf("%08x\n", xor_running);
  }

  free(buffer);
  close(fd);

  return xor_running;
}

void *readFileThread(void *arg) {
  struct readArgs *args = (struct readArgs *)arg;

  long xor = (long)readFile(args->filename, args->blockSize, args->blockCount,
                            args->offset, 1);

  pthread_exit((void *)xor);
}

int readFileFast(char *filename, long blockSize, long totalBlockCount,
                 int numThreads, int quiet) {
  off_t totalFileSize;

  if (totalBlockCount == -1) {
    // read entire file, not a specific block count
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

  if (totalFileSize < (numThreads * blockSize)) {
    if (totalBlockCount != -1) {
      // only print this error message if a specific block count was passed
      // to the function
      fprintf(stderr, "block count * block size < num threads * block size\n");
    }

    long blockCount = (long)ceill(totalFileSize / (long double)blockSize);
    readFile(filename, blockSize, blockCount, 0, quiet);
    exit(1);
  }

  off_t idealBytesPerThread =
      bytesPerInt * (totalFileSize / (numThreads * bytesPerInt));

  off_t blockCountPerThread = idealBytesPerThread / blockSize;

  off_t realBytesPerThread = blockSize * blockCountPerThread;

  pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);
  if (threads == NULL) {
    fprintf(stderr, "failed to allocate memory for threads\n");
    exit(1);
  }
  struct readArgs *args[numThreads];

  for (int i = 0; i < numThreads; i++) {
    off_t offset = i * realBytesPerThread;

    long blockCount = blockCountPerThread;

    if (i == numThreads - 1) {
      off_t bytesLeft = totalFileSize - offset;
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

    int threadCreateRet =
        pthread_create(&threads[i], NULL, readFileThread, args[i]);
    if (threadCreateRet != 0) {
      fprintf(stderr, "failed create thread %d\n", i);
      exit(1);
    }
  }

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

// ./run <filename> [-r|-w] <block_size> <block_count> [-q]
// ./run <filename> -f <block_size> <num_threads> [-q]
// ./run <filename> -t <block_size> <num_threads> <block_count>
int main(int argc, char **argv) {
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
  int quiet = 0;

  // if the mode is -r, -w, or -t, then the 6th
  // argument can only be the quiet flag
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
    readFileFast(filename, blockSize, -1, numThreads, quiet);
  } else if (strcmp(argv[2], "-t") == 0) {
    // fast read specific block count
    long blockSize = atol(argv[3]);
    int numThreads = atoi(argv[4]);
    int numBlocks = atoi(argv[5]);
    if (blockSize <= 0 || numThreads <= 0 || numBlocks <= 0) {
      fprintf(stderr,
              "invalid block size, number of threads, or block count\n");
      return 1;
    }
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