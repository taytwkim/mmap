#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define FILE_NAME "mmap_file.txt"
#define FILE_SIZE 8192

/*
 * What is a memory-mapped file?
 *
 * With normal read(), file data is brought into kernel memory,
 * and then copied into the user-space buffer we provided.
 *
 * This means the same file data can exist in two places:
 * one copy in kernel memory, and another copy in our
 * user-space buffer.
 * 
 * We can index into the buffer using offsets, but we have to be
 * careful not to read beyond the range that was actually copied
 * into the buffer.
 * 
 * A memory-mapped file can be thought of as dividing a large file
 * into pages and accessing them like normal memory.
 *
 * If we access a part of the mapped file whose page is not currently
 * in memory, the normal OS page fault mechanism can bring that page in.
 *
 * So it is useful to think of a memory-mapped file as treating the file
 * as a collection of pages that can be randomly accessed.
 */
int main(void) {
    /*
     * Create a file and write contents at specific offsets.
     * 
     * O_RDWR   -> open for both reading and writing
     * O_CREAT  -> create the file if it does not exist
     * O_TRUNC  -> if it already exists, erase its old contents
     * 0644     -> permissions for the new file if it is created
     */
    int fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open");
        exit(1);
    }

    // Set the file size to FILE_SIZE to make sure its big enough.
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
        MAP_SHARED, // Use MAP_SHARED to modify the actual file.
        fd,
        0
    );

    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    printf("p + 0:    %s", p + 0);
    printf("p + 4096: %s", p + 4096);

    // Modify the file.
    // Note that we are writing to memory, not using write().
    strcpy(p + 4096, "updated through mmap\n");

    // Ask OS to flush the modified mapped memory back to the actual file now.
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