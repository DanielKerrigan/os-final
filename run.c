#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int ERROR_EXIT_STATUS = 1;

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
    return ERROR_EXIT_STATUS;
  }

  if (offset != 0) {
    off_t lseekOffset = lseek(fd, offset, SEEK_SET);
  }

  unsigned int *buffer = malloc(blockSize);
  unsigned int xor_running = 0;

  size_t bytesRead;

  for (int i = 0; i < blockCount; i++) {
    bytesRead = read(fd, buffer, blockSize);
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

int readFileFast(char *filename) {
  struct stat statbuf;
  stat(filename, &statbuf);

  off_t totalFileSize = statbuf.st_size;

  int numThreads = 4;
  int bytesPerInt = 4;

  long blockSize = 1024;

  off_t idealBytesPerThread =
      bytesPerInt * (totalFileSize / (numThreads * bytesPerInt));

  off_t blockCountPerThread = idealBytesPerThread / blockSize;

  off_t realBytesPerThread = blockSize * blockCountPerThread;

  pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);

  struct readArgs *args [numThreads];

  for (int i = 0; i < numThreads; i++) {
    off_t offset = i * realBytesPerThread;

    long blockCount = blockCountPerThread;

    if (i == numThreads - 1) {
      off_t bytesLeft = totalFileSize - offset;
      blockCount = (long)ceil((double)bytesLeft / blockSize);
    }

    args[i] = malloc(sizeof(struct readArgs));

    args[i]->filename = filename;
    args[i]->blockSize = blockSize;
    args[i]->blockCount = blockCount;
    args[i]->offset = offset;
    args[i]->id = i;

    pthread_create(&threads[i], NULL, readFileThread, args[i]);
  }

  long xor = 0;

  for (int i = 0; i < numThreads; i++) {
    long xorThread;
    pthread_join(threads[i], (void **)&xorThread);

    xor = xor^xorThread;
  }

  printf("%08lx\n", xor);

  for(int i = 0; i < numThreads; i++){
    free(args[i]);
  }
  
  free(threads);

  return 0;
}

int writeFile(char *filename, long blockSize, long blockCount) {
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd == -1) {
    perror("open");
  }

  unsigned int *buffer = malloc(blockSize);

  for (int i = 0; i < blockCount; i++) {
    write(fd, buffer, blockSize);
  }

  free(buffer);
  close(fd);

  return 0;
}

// ./run <filename> [-r|-w] <block_size> <block_count> [-q]
int main(int argc, char **argv) {
  char *filename = argv[1];
  char mode = argv[2][1];
  int quiet = 0;

  if (mode == 'f') {
    return readFileFast(filename);
  } else {
    long blockSize = atol(argv[3]);
    long blockCount = atol(argv[4]);

    if (argc == 6 && argv[5][1] == 'q') {
      quiet = 1;
    }

    if (mode == 'r') {
      return readFile(filename, blockSize, blockCount, 0, quiet);
    } else if (mode == 'w') {
      return writeFile(filename, blockSize, blockCount);
    } else {
      fprintf(stderr, "unknown flag -%c\n", mode);
      return ERROR_EXIT_STATUS;
    }
  }
}