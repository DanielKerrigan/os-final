# CS-GY 6233 Final Project

Racquel Fygenson (rlf9859)

Daniel Kerrigan (djk525)

## Instructions

### Compiling

To compile our code, you can run `./build`.

### Part 1

Reading and writing files using a single thread can be done with the following command:

```
./run <filename> [-r|-w] <block_size> <block_count>
```

### Part 2

To find the reasonable block count for a given block size, you can run the following command:

```
./run2 <filename> <block_size>
```

### Part 3 and 4

To measure the performance of cold cache reads for various block sizes, the following command can be used:

````
sudo ./benchmark --perf_no_cache <filename>
```

To measure the performance of warm cache reads for various block sizes, the following command can be used:

```
./benchmark --perf_cache <filename>
```

### Part 5

To measure the performance of `fstat`, `lseek`, `getuid`, and `getpid`, the following command can be used:

```
./benchmark --sys_calls <filename>
```

### Part 6

To measure the performance of cold cache reads for various block sizes and number of threads, the following command can be used:

```
sudo ./benchmark --perf_no_cache_threads <filename>
```

To measure the performance of warm cache reads for various block sizes and number of threads, the following command can be used:

```
./benchmark --perf_cache_threads <filename>
```

To run the fast read version of our `./run` program, which uses 8 threads and 32 KiB block size, the following command can be used:

```
./fast <file_to_read>
```