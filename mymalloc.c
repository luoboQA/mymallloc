/*
 * Simple malloc/free implementation
 * Based on danluu.com/malloc-tutorial/
 * 
 * Compile: gcc -O0 -g -Wall -Wextra -shared -fPIC mymalloc.c -o mymalloc.so
 * Use: LD_PRELOAD=./mymalloc.so ls
 */

#define _GNU_SOURCE
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

/* Memory block header structure */
typedef struct block_meta {
    size_t size;                /* Size of the block (excluding header) */
    struct block_meta *next;    /* Next block in linked list */
    int free;                   /* Is the block free? 1=free, 0=allocated */
    int magic;                   /* Magic number for debugging */
} block_meta_t;

#define META_SIZE sizeof(block_meta_t)
#define MAGIC_FREE 0x55555555 // 空闲块标记
#define MAGIC_ALLOC 0x77777777  // 已分配块标记  
#define MAGIC_NEW 0x12345678 // 新分配块的标记
#define ALIGNMENT 8  /* 8-byte alignment for 64-bit systems 地址对齐*/

/* Head of the free list */
void *global_base = NULL;

/**
 * Align size to nearest multiple of ALIGNMENT
 */
size_t align_size(size_t size) {
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

/**
 * Get pointer to block header from user pointer
 */
block_meta_t *get_block_ptr(void *ptr) {
    return (block_meta_t*)ptr - 1;
}

/**
 * Find a free block that can fit the requested size
 * Uses first-fit algorithm
 * block_meta_t **last - 双重指针，记录最后一个检查的块，以便在需要请求新内存时链接新块
 */
block_meta_t *find_free_block(block_meta_t **last, size_t size) {
     // 1. 从头开始遍历链表，寻找第一个满足条件的空闲块（free=1且size>=请求的size）
    block_meta_t *current = global_base;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

/**
 * Request more memory from OS using sbrk,no free block
 */
block_meta_t *request_space(block_meta_t* last, size_t size) {
    block_meta_t *block;
    //获取当前堆顶位置
    block = sbrk(0);
    
    /* Request more memory (header + requested size) */
    void *request = sbrk(size + META_SIZE);
    
    /* Check for sbrk failure */
    if (request == (void*) -1) {
        return NULL;
    }
    
    /* Verify sbrk returned expected address (not thread-safe) */
    assert((void*)block == request);
    
    /* Link the new block */
    if (last) {
        last->next = block;
    }
    
    /* Initialize block header */
    block->size = size;
    block->next = NULL;
    block->free = 0;
    block->magic = MAGIC_NEW;
    
    return block;
}

/**
 * Split a block if it's larger than needed
 */
void split_block(block_meta_t *block, size_t size) {
    /* Only split if remaining space can hold a header + minimum data */
    if (block->size > size + META_SIZE + ALIGNMENT) {
        block_meta_t *new_block = (block_meta_t*)((char*)block + META_SIZE + size);
        
        new_block->size = block->size - size - META_SIZE;
        new_block->next = block->next;
        new_block->free = 1;
        new_block->magic = MAGIC_FREE;
        
        block->size = size;
        block->next = new_block;
    }
}

/**
 * malloc - allocate memory
 */
void *malloc(size_t size) {
    block_meta_t *block;
    
    /* Handle zero-size requests */
    if (size <= 0) {
        return NULL;
    }
    
    /* Align size for better performance */
    size = align_size(size);
    
    /* First call - initialize */
    if (!global_base) {
        block = request_space(NULL, size);
        if (!block) {
            return NULL;
        }
        global_base = block;
    } else {
        block_meta_t *last = global_base;
        block = find_free_block(&last, size);
        
        if (!block) {
            /* No suitable free block found, request more memory */
            block = request_space(last, size);
            if (!block) {
                return NULL;
            }
        } else {
            /* Found a free block */
            split_block(block, size);
            block->free = 0;
            block->magic = MAGIC_ALLOC;
        }
    }
    
    /* Return pointer after the header */
    return (block + 1);
}

/**
 * free - release allocated memory
 */
void free(void *ptr) {
    if (!ptr) {
        return;
    }
    
    /* Get block header */
    block_meta_t* block_ptr = get_block_ptr(ptr);
    
    /* Verify it's a valid block */
    assert(block_ptr->free == 0);
    assert(block_ptr->magic == MAGIC_ALLOC || block_ptr->magic == MAGIC_NEW);
    
    /* Mark as free */
    block_ptr->free = 1;
    block_ptr->magic = MAGIC_FREE;
    
    /* Optional: Could merge adjacent free blocks here */
}

/**
 * Merge adjacent free blocks to reduce fragmentation
 */
void merge_free_blocks() {
    block_meta_t *current = global_base;
    
    while (current && current->next) {
        if (current->free && current->next->free) {
            /* Merge current and next block */
            current->size += META_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

/**
 * realloc - resize allocated memory
 */
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        /* NULL pointer - act like malloc */
        return malloc(size);
    }
    
    /* Handle size=0 - act like free */
    if (size <= 0) {
        free(ptr);
        return NULL;
    }
    
    /* Align size */
    size = align_size(size);
    
    /* Get block header */
    block_meta_t* block_ptr = get_block_ptr(ptr);
    
    /* If current block is large enough */
    if (block_ptr->size >= size) {
        split_block(block_ptr, size);
        return ptr;
    }
    
    /* Need to allocate new space */
    void *new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    /* Copy old data */
    memcpy(new_ptr, ptr, block_ptr->size);
    
    /* Free old block */
    free(ptr);
    
    return new_ptr;
}

/**
 * calloc - allocate and zero-initialize memory
 */
void *calloc(size_t nelem, size_t elsize) {
    /* Check for overflow */
    if (nelem == 0 || elsize == 0) {
        return NULL;
    }
    
    size_t size = nelem * elsize;
    
    /* Check for multiplication overflow */
    if (nelem != size / elsize) {
        return NULL; /* Overflow occurred */
    }
    
    void *ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

/**
 * aligned_alloc - allocate aligned memory
 */
void *aligned_alloc(size_t alignment, size_t size) {
    /* Alignment must be power of 2 and multiple of sizeof(void*) */
    if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }
    
    /* Align size to multiple of alignment */
    size = (size + alignment - 1) & ~(alignment - 1);
    
    /* Allocate extra space for alignment adjustment */
    void *ptr = malloc(size + alignment);
    if (!ptr) {
        return NULL;
    }
    
    /* Calculate aligned address */
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    
    if (addr == aligned_addr) {
        /* Already aligned */
        return ptr;
    }
    
    /* We need to store the original pointer somewhere */
    /* For simplicity, we'll just use the block before the aligned address */
    void **aligned_ptr = (void**)aligned_addr;
    aligned_ptr[-1] = ptr;
    
    return aligned_ptr;
}

/**
 * free_aligned - free memory allocated with aligned_alloc
 */
void free_aligned(void *ptr) {
    if (!ptr) {
        return;
    }
    
    /* Retrieve original pointer */
    void **aligned_ptr = (void**)ptr;
    void *original_ptr = aligned_ptr[-1];
    
    free(original_ptr);
}

/**
 * malloc_usable_size - get size of allocated block
 */
size_t malloc_usable_size(void *ptr) {
    if (!ptr) {
        return 0;
    }
    
    block_meta_t *block_ptr = get_block_ptr(ptr);
    return block_ptr->size;
}

/**
 * Debug function: print memory map
 */
void print_memory_map(void) {
    block_meta_t *current = global_base;
    int i = 0;
    
    printf("\n=== Memory Map ===\n");
    while (current) {
        printf("Block %d: addr=%p, size=%zu, free=%d, magic=0x%x\n",
               i++, (void*)current, current->size, current->free, current->magic);
        current = current->next;
    }
    printf("==================\n\n");
}

/**
 * Get heap statistics
 */
void malloc_stats(void) {
    block_meta_t *current = global_base;
    size_t total = 0, used = 0, free_space = 0;
    int blocks = 0, free_blocks = 0;
    
    while (current) {
        blocks++;
        total += current->size + META_SIZE;
        
        if (current->free) {
            free_blocks++;
            free_space += current->size + META_SIZE;
        } else {
            used += current->size + META_SIZE;
        }
        
        current = current->next;
    }
    
    printf("Heap Statistics:\n");
    printf("  Total blocks: %d\n", blocks);
    printf("  Free blocks: %d\n", free_blocks);
    printf("  Total size: %zu bytes\n", total);
    printf("  Used: %zu bytes\n", used);
    printf("  Free: %zu bytes\n", free_space);
    printf("  Fragmentation: %.2f%%\n", 
           free_blocks > 0 ? (float)free_blocks / blocks * 100 : 0);
}