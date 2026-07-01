#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define FILE_NAME "mmap_test.txt"
#define FILE_SIZE 8192

int main(void) {
    int fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    if (ftruncate(fd, FILE_SIZE) == -1) {
        perror("ftruncate");
        exit(1);
    }

    pwrite(fd, "hello from offset 0\n", 20, 0);
    pwrite(fd, "hello from offset 4096\n", 23, 4096);

    printf("=== normal read ===\n");

    char buf[64] = {0};

    if (pread(fd, buf, sizeof(buf) - 1, 4096) == -1) {
        perror("pread");
        exit(1);
    }

    printf("buf: %s", buf);

    printf("\n=== file-backed mmap ===\n");

    char* p = mmap(
        NULL,
        FILE_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    printf("p + 0:    %s", p + 0);
    printf("p + 4096: %s", p + 4096);

    strcpy(p + 4096, "updated through mmap\n");

    if (msync(p, FILE_SIZE, MS_SYNC) == -1) {
        perror("msync");
        exit(1);
    }

    munmap(p, FILE_SIZE);
    close(fd);

    printf("\nNow run:\n");
    printf("cat %s\n", FILE_NAME);

    return 0;
}