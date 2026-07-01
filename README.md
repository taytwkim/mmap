# Memory Mapping in Linux

Short exercises to get familiar with memory mapping in Linux.

`mmap.c` demonstrates the usage of `sbrk` and `mmap`. `mmap_file.c` demonstrates the usage of memory-mapped files.

## Getting Started

Compile and run:

```shell
gcc -Wall -Wextra -O0 -g mmap.c -o mmap
```

```shell
gcc -Wall -Wextra -O0 -g mmap_file.c -o mmap_file
```