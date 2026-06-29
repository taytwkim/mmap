#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int global_var = 123;
static int static_var = 456;

void sample_function(void) {
    // Used only so we can print a function address.
}

void wait_enter(const char* message) {
    printf("\n=== %s ===\n", message);
    printf("Press Enter to continue...\n");
    getchar();
}

void dump_maps(const char* label) {
    printf("\n\n========== /proc/self/maps: %s ==========\n", label);

    FILE* f = fopen("/proc/self/maps", "r");
    if (f == NULL) {
        perror("fopen");
        exit(1);
    }

    char line[512];

    while (fgets(line, sizeof(line), f) != NULL) {
        printf("%s", line);
    }

    fclose(f);

    printf("========== end maps ==========\n\n");
}

int main(void) {
    int stack_var = 789;

    printf("pid: %d\n", getpid());

    printf("\n--- basic addresses ---\n");
    printf("function address:   %p\n", (void*)sample_function);
    printf("global_var address: %p\n", (void*)&global_var);
    printf("static_var address: %p\n", (void*)&static_var);
    printf("stack_var address:  %p\n", (void*)&stack_var);

    printf("\n--- program break ---\n");
    printf("program break at start: %p\n", sbrk(0));

    wait_enter("initial memory map");
    dump_maps("initial");

    wait_enter("small malloc: malloc(1024)");
    void* small = malloc(1024);
    if (small == NULL) {
        perror("malloc small");
        exit(1);
    }

    printf("small malloc address: %p\n", small);
    printf("program break after small malloc: %p\n", sbrk(0));

    dump_maps("after small malloc");

    wait_enter("large malloc: malloc(10 MB)");
    void* large = malloc(10 * 1024 * 1024);
    if (large == NULL) {
        perror("malloc large");
        exit(1);
    }

    printf("large malloc address: %p\n", large);
    printf("program break after large malloc: %p\n", sbrk(0));

    dump_maps("after large malloc");

    wait_enter("anonymous mmap: mmap(8192 bytes)");
    size_t mmap_size = 4096 * 2;

    void* mapped = mmap(
        NULL,
        mmap_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (mapped == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    printf("mmap address: %p\n", mapped);

    strcpy((char*)mapped, "hello from mmap");
    printf("mapped content: %s\n", (char*)mapped);

    dump_maps("after mmap");

    wait_enter("munmap");
    if (munmap(mapped, mmap_size) == -1) {
        perror("munmap");
        exit(1);
    }

    dump_maps("after munmap");

    wait_enter("free mallocs");
    free(small);
    free(large);

    printf("program break after free: %p\n", sbrk(0));

    dump_maps("after free");

    return 0;
}