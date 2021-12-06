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

int readFile(char* filename, long blockSize, long blockCount, int quiet) {
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

  if(!quiet){
    printf("%08x\n", xor_running);
  }

  return 0;
}

int readFileFast(char* filename){
  int fd = open(filename, O_RDONLY);
  int blockSize = 1024;
  int bytesRead = 0;

  if (fd == -1) {
    perror("open");
    return ERROR_EXIT_STATUS;
  }

  unsigned int* buffer = malloc(blockSize);
  unsigned int xor_running = 0;

  while((bytesRead = read(fd, buffer, blockSize)) > 0){
    xor_running = xor_running ^ xorbuf(buffer, bytesRead);
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


// ./run <filename> [-r|-w] <block_size> <block_count> [-q]
int main(int argc, char **argv) {
  char* filename = argv[1];
  char mode = argv[2][1];
  int quiet = 0;

  if(mode =='f'){
    return readFileFast(filename);
  }
  else{
    long blockSize = atol(argv[3]);
    long blockCount = atol(argv[4]);

    if(argc == 6 && argv[5][1]=='q'){
      quiet = 1;
    }

    if (mode == 'r') {
      return readFile(filename, blockSize, blockCount, quiet);
    } else if (mode == 'w'){
      return writeFile(filename, blockSize, blockCount);
    } else {
      fprintf(stderr, "unknown flag -%c\n", mode);
      return ERROR_EXIT_STATUS;
    }
  }  
}