/*
 * What is _GNU_SOURCE?
 *
 * _GNU_SOURCE is a macro used by glibc.
 * It tells the header files to expose GNU/Linux-specific
 * extensions in addition to the standard C/POSIX declarations.
 */
#define _GNU_SOURCE

#include <stdio.h>      // printf, fopen, fgets, etc.
#include <stdlib.h>     // malloc, free, exit, etc.
#include <unistd.h>     // "Unix standard": getpid, sbrk
#include <sys/mman.h>   // "memory management": mmap, munmap, ...
#include <string.h>     // strcpy

// Both global and static variables go to .data segment.
int global_var = 123;
static int static_var = 456;

void sample_function(void) {
    // Used only so we can print a function address.
}

// Used to insert break points
void wait_enter(const char* message) {
    printf("\n=== %s ===\n", message);
    printf("Press Enter to continue...\n");
    getchar();
}

void dump_maps(const char* label) {
    printf("\n\n========== /proc/self/maps: %s ==========\n", label);

    /*
     * /proc is a kernel-provided interface that lets programs
     * inspect process and system info as if reading from files.
     *
     * /proc/self/maps shows the virtual memory regions of the process
     * that opens it.
     */
    FILE* f = fopen("/proc/self/maps", "r");

    if (f == NULL) {
        /*
         * perror means print error
         * perror prints a custom label followed by a human-readable message 
         * for the most recent error stored in errno.
         */
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
    printf("function address:   %p\n", (void*)sample_function); // void* is a pointer to an object of unknown type.
    printf("global_var address: %p\n", (void*)&global_var);
    printf("static_var address: %p\n", (void*)&static_var);
    printf("stack_var address:  %p\n", (void*)&stack_var);

    /*
     * What are brk/sbrk?
     *
     * brk/sbrk are functions that modify the "program break,"
     * which is the end of the traditional brk-managed heap.
     *
     * brk(addr) sets the program break to addr.
     * sbrk(x) shifts the program break by x bytes.
     *
     * sbrk shifts the program break and returns the program break
     * address from before the shift. sbrk(0) is a common
     * way to retrieve the current program break without changing it.
     */
    printf("\n--- program break ---\n");
    printf("program break at start: %p\n", sbrk(0));

    wait_enter("initial memory map");
    dump_maps("initial");

    /*
     * Below, we check whether malloc changed the program break.
     *
     * This does not assume that malloc always uses brk/sbrk.
     * malloc may reuse memory already managed by the allocator,
     * extend the traditional brk-managed heap, or use a separate
     * mmap region.
     *
     * Comparing sbrk(0) before and after malloc only tells us
     * whether the brk-managed heap moved.
     *
     * Also note that calling brk/sbrk does not necessarily map 
     * the new virtual pages to physical frames right away.
     *
     * Instead, the kernel records the expanded heap range as valid
     * virtual memory for this process. Physical frames may be allocated
     * later, when the process first touches those pages and a page fault
     * occurs.
     */
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

    /*
     * What is mmap?
     *
     * In the textbook virtual memory address layout,
     * the stack and heap are often shown as growing toward each other.
     *
     * In reality, a process can have separate memory mappings between
     * the stack and the heap, and mmap can create mappings in this area.
     * 
     * So dynamically allocated memory does not necessarily come from
     * the top of the traditional heap.
     */
    wait_enter("anonymous mmap: mmap(8192 bytes)");

    size_t mmap_size = 4096 * 2;

    /*
     * MMAP Input Arguments
     * 
     * 1. NULL: We don't care where the mapping goes/starts.
     *
     * 2. mmap_size: Note that memory is mapped at page granularity.
     *      If the requested size is not a multiple of the page size,
     *      the kernel will round up and map enough pages to cover the 
     *      requested range.
     * 
     * 3. PROT_READ | PROT_WRITE: Protection permissions.
     *      We can use bitwise OR (|) to combine multiple flags.
     * 
     * 4. MAP_PRIVATE | MAP_ANONYMOUS: Mapping flags.
     *      Private means private to this process.
     *      Anonymous means the mapping is not backed by a file
     *      and the memory can be zero-initialized.
     *
     *      -1 and 0 are placeholders for file descriptor/offset, which
     *      needs to be specified if memory is file-mapped.
     */
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