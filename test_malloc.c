/*
 * Test program for custom malloc implementation
 * 
 * Compile: gcc -o test_malloc test_malloc.c mymalloc.c
 * Run: ./test_malloc
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* External declarations for our malloc functions */
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *calloc(size_t nelem, size_t elsize);
extern void *realloc(void *ptr, size_t size);
extern void print_memory_map(void);
extern void malloc_stats(void);

/* Test function prototypes */
void test_basic_allocation(void);
void test_array_allocation(void);
void test_string_operations(void);
void test_calloc_initialization(void);
void test_realloc_grow(void);
void test_realloc_shrink(void);
void test_memory_reuse(void);
void test_stress_allocation(void);

int main() {
    printf("=== Custom Malloc Test Suite ===\n\n");
    
    test_basic_allocation();
    test_array_allocation();
    test_string_operations();
    test_calloc_initialization();
    test_realloc_grow();
    test_realloc_shrink();
    test_memory_reuse();
    test_stress_allocation();
    
    printf("\nAll tests completed!\n");
    malloc_stats();
    
    return 0;
}

void test_basic_allocation(void) {
    printf("--- Basic Allocation Test ---\n");
    
    int *p = (int*)malloc(sizeof(int));
    if (!p) {
        printf("  FAIL: malloc failed\n");
        return;
    }
    
    *p = 42;
    printf("  Allocated int: %d (addr: %p)\n", *p, (void*)p);
    
    free(p);
    printf("  Freed successfully\n\n");
}

void test_array_allocation(void) {
    printf("--- Array Allocation Test ---\n");
    
    int *arr = (int*)malloc(10 * sizeof(int));
    if (!arr) {
        printf("  FAIL: malloc failed\n");
        return;
    }
    
    for (int i = 0; i < 10; i++) {
        arr[i] = i * i;
    }
    
    printf("  Array contents: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    
    free(arr);
    printf("  Freed successfully\n\n");
}

void test_string_operations(void) {
    printf("--- String Operations Test ---\n");
    
    char *str = (char*)malloc(20);
    if (!str) {
        printf("  FAIL: malloc failed\n");
        return;
    }
    
    strcpy(str, "Hello, World!");
    printf("  String: %s (addr: %p)\n", str, (void*)str);
    
    free(str);
    printf("  Freed successfully\n\n");
}

void test_calloc_initialization(void) {
    printf("--- Calloc Initialization Test ---\n");
    
    int *arr = (int*)calloc(10, sizeof(int));
    if (!arr) {
        printf("  FAIL: calloc failed\n");
        return;
    }
    
    int all_zero = 1;
    for (int i = 0; i < 10; i++) {
        if (arr[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    
    printf("  All elements zero: %s\n", all_zero ? "YES" : "NO");
    
    free(arr);
    printf("  Freed successfully\n\n");
}

void test_realloc_grow(void) {
    printf("--- Realloc Grow Test ---\n");
    
    char *str = (char*)malloc(10);
    if (!str) {
        printf("  FAIL: malloc failed\n");
        return;
    }
    
    strcpy(str, "Hello");
    printf("  Original string: %s (size: 10)\n", str);
    
    str = (char*)realloc(str, 20);
    if (!str) {
        printf("  FAIL: realloc failed\n");
        return;
    }
    
    strcat(str, " World!");
    printf("  After realloc: %s (size: 20)\n", str);
    
    free(str);
    printf("  Freed successfully\n\n");
}

void test_realloc_shrink(void) {
    printf("--- Realloc Shrink Test ---\n");
    
    int *arr = (int*)malloc(100 * sizeof(int));
    if (!arr) {
        printf("  FAIL: malloc failed\n");
        return;
    }
    
    for (int i = 0; i < 100; i++) {
        arr[i] = i;
    }
    
    printf("  Original array size: 100 ints\n");
    
    arr = (int*)realloc(arr, 50 * sizeof(int));
    if (!arr) {
        printf("  FAIL: realloc failed\n");
        return;
    }
    
    printf("  After shrink - first 5 elements: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    
    free(arr);
    printf("  Freed successfully\n\n");
}

void test_memory_reuse(void) {
    printf("--- Memory Reuse Test ---\n");
    
    void *ptrs[5];
    
    printf("  Allocating blocks:\n");
    for (int i = 0; i < 5; i++) {
        ptrs[i] = malloc(64);
        printf("    Block %d: %p\n", i, ptrs[i]);
    }
    
    printf("  Freeing middle blocks:\n");
    free(ptrs[1]);  /* Free second block */
    free(ptrs[3]);  /* Free fourth block */
    printf("    Freed blocks 1 and 3\n");
    
    void *new_ptr = malloc(32);
    printf("  New allocation (32 bytes): %p\n", new_ptr);
    
    /* Check if new_ptr reused one of the freed blocks */
    int reused = 0;
    for (int i = 0; i < 5; i++) {
        if (i == 1 || i == 3) {  /* Freed blocks */
            if (new_ptr == ptrs[i]) {
                reused = 1;
                break;
            }
        }
    }
    
    printf("  Memory reused: %s\n", reused ? "YES" : "NO");
    
    /* Clean up */
    for (int i = 0; i < 5; i++) {
        if (i != 1 && i != 3) {  /* Skip already freed blocks */
            free(ptrs[i]);
        }
    }
    free(new_ptr);
    printf("\n");
}

void test_stress_allocation(void) {
    printf("--- Stress Allocation Test ---\n");
    
    #define NUM_ALLOCS 100
    #define MAX_SIZE 1024
    
    void *ptrs[NUM_ALLOCS];
    size_t sizes[NUM_ALLOCS];
    
    printf("  Performing %d random allocations/frees...\n", NUM_ALLOCS);
    
    for (int i = 0; i < NUM_ALLOCS; i++) {
        int action = rand() % 3;
        
        if (action == 0 || i < 10) {  /* Allocate */
            sizes[i] = (rand() % MAX_SIZE) + 1;
            ptrs[i] = malloc(sizes[i]);
            if (ptrs[i]) {
                /* Write some pattern to memory */
                memset(ptrs[i], i % 256, sizes[i]);
            }
        } else if (action == 1 && i > 0) {  /* Free */
            int idx = rand() % i;
            if (ptrs[idx]) {
                free(ptrs[idx]);
                ptrs[idx] = NULL;
            }
            i--;  /* Retry this iteration */
        } else {  /* Realloc */
            int idx = rand() % i;
            if (ptrs[idx]) {
                size_t new_size = (rand() % MAX_SIZE) + 1;
                ptrs[idx] = realloc(ptrs[idx], new_size);
                if (ptrs[idx]) {
                    sizes[idx] = new_size;
                }
            }
        }
    }
    
    /* Clean up */
    int freed_count = 0;
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (ptrs[i]) {
            free(ptrs[i]);
            freed_count++;
        }
    }
    
    printf("  Stress test complete. Freed %d remaining blocks.\n", freed_count);
    
    /* Show final heap state */
    malloc_stats();
    printf("\n");
}