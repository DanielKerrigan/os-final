#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int ERROR_EXIT_STATUS = 1;

int readFile(char* filename, int blockSize, int blockCount) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return ERROR_EXIT_STATUS;
  }

  return 0;
}

int writeFile(char* filename, int blockSize, int blockCount) {
  int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (fd == -1) {
    perror("open");
  }

  void* buffer = malloc(blockSize);

  for (int i = 0; i < blockCount; i++) {
    write(fd, buffer, blockSize);
  }

  close(fd);

  return 0;
}

// ./run <filename> [-r|-w] <block_size> <block_count>
int main(int argc, char **argv) {
  char* filename = argv[1];
  char readOrWrite = argv[2][1];
  int blockSize = atoi(argv[3]);
  int blockCount = atoi(argv[4]);

  if (readOrWrite == 'r') {
    return readFile(filename, blockSize, blockCount);
  } else if (readOrWrite == 'w'){
    return writeFile(filename, blockSize, blockCount);
  } else {
    fprintf(stderr, "unknown flag -%c\n", readOrWrite);
    return ERROR_EXIT_STATUS;
  }
}