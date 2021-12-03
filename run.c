#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int ERROR_EXIT_STATUS = 1;

unsigned int xorbuf(unsigned int *buffer, int size) {
    unsigned int result = 0;
    for (int i = 0; i < size; i++) {
        result ^= buffer[i];
    }
    return result;
}

int readFile(char* filename, long blockSize, long blockCount) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return ERROR_EXIT_STATUS;
  }

  unsigned int* buffer = malloc(blockSize);
  unsigned int xor_running = 0;

  for (int i = 0; i < blockCount; i++) {
    read(fd, buffer, blockSize);
    xor_running = xor_running ^ xorbuf(buffer, blockSize);
  }

  printf("%08x\n", xor_running);

  return 0;
}

int writeFile(char* filename, long blockSize, long blockCount) {
  int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (fd == -1) {
    perror("open");
  }

  unsigned int* buffer = malloc(blockSize);

  for (int i = 0; i < blockCount; i++) {
    write(fd, buffer, blockSize);
  }

  free(buffer);
  close(fd);

  return 0;
}


// ./run <filename> [-r|-w] <block_size> <block_count>
int main(int argc, char **argv) {
  char* filename = argv[1];
  char readOrWrite = argv[2][1];

  long blockSize = atol(argv[3]);
  long blockCount = atol(argv[4]);

  if (readOrWrite == 'r') {
    return readFile(filename, blockSize, blockCount);
  } else if (readOrWrite == 'w'){
    return writeFile(filename, blockSize, blockCount);
  } else {
    fprintf(stderr, "unknown flag -%c\n", readOrWrite);
    return ERROR_EXIT_STATUS;
  }
}