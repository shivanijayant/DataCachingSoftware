//#define DEBUG
#include <stdio.h>
#include <stdbool.h>

#ifdef DEBUG
#define debug_printf(...) printf(__VA_ARGS__)
#define debug_assert(...) assert(__VA_ARGS__)
#else
#define debug_printf(...)
#define debug_assert(...)
#endif

#ifdef DRIVER
#endif

#define ALIGNMENT 16
#define MIN_BLOCK_SIZE 32

typedef struct {
    size_t block_size;
    struct free_block *next_free;
    struct free_block *prev_free;
} free_block;

typedef struct {
    size_t block_size;
} allocated_block;

typedef size_t block_footer;

static size_t align(size_t size) {
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

static size_t free_block_header_size() {
    return align(sizeof(free_block) + sizeof(block_footer));
}

static size_t allocated_block_header_size() {
    return align(sizeof(allocated_block) + sizeof(block_footer));
}

bool mm_init(void) {
    free_block *initial_free_block = mem_sbrk(free_block_header_size());
    if (initial_free_block == (void *)-1)
        return false;

    initial_free_block->block_size = free_block_header_size();
    initial_free_block->next_free = initial_free_block;
    initial_free_block->prev_free = initial_free_block;

    block_footer *footer = (block_footer *)((char *)initial_free_block + initial_free_block->block_size - sizeof(block_footer));
    *footer = initial_free_block->block_size;

    return true;
}

void *mm_malloc(size_t size) {
    if (size == 0)
        return NULL;

    size_t required_size = align(size + sizeof(allocated_block));
    required_size = (required_size < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : required_size;

    free_block *block = find_fit(required_size);
    if (block == NULL) {
        block = mem_sbrk(required_size);
        if (block == (void *)-1)
            return NULL;
        block->block_size = required_size | 1;
    } else {
        split_free_block(block, required_size);
    }

    return (char *)block + sizeof(allocated_block);
}

void mm_free(void *ptr) {
    if (ptr == NULL)
        return;

    free_block *block = (free_block *)((char *)ptr - sizeof(allocated_block));
    block->block_size &= ~1;

    free_block *head = (free_block *)((char *)mem_heap_lo() + sizeof(size_t));
    block->next_free = head->next_free;
    block->prev_free = head;
    head->next_free->prev_free = block;
    head->next_free = block;

    coalesce_free_blocks(block);
}

void *mm_realloc(void *oldptr, size_t size) {
    // Implementation for realloc function
}
static void coalesce_free_blocks(free_block *block) {
    char *block_end = (char *)block + block->block_size;
    free_block *next_block = (free_block *)block_end;

    // Coalesce with next block if it's free
    if ((char *)next_block < mem_heap_hi() && !(next_block->block_size & 1)) {
        block->block_size += next_block->block_size;
        block->next_free = next_block->next_free;
        block->prev_free = next_block->prev_free;
        next_block->next_free->prev_free = block;
        next_block->prev_free->next_free = block;
    }

    // Coalesce with previous block if it's free
    if (block != mem_heap_lo() + sizeof(size_t)) {
        block_footer *prev_footer = (block_footer *)((char *)block - sizeof(block_footer));
        size_t prev_block_size = *prev_footer;
        free_block *prev_block = (free_block *)((char *)block - prev_block_size);
        if (!(prev_block->block_size & 1)) {
            prev_block->block_size += block->block_size;
            prev_block->next_free = block->next_free;
            prev_block->prev_free = block->prev_free;
            block->next_free->prev_free = prev_block;
            block->prev_free->next_free = prev_block;
        }
    }
}

static void split_free_block(free_block *block, size_t required_size) {
    size_t block_size = block->block_size;
    if (block_size - required_size >= MIN_BLOCK_SIZE) {
        block->block_size = required_size;
        free_block *new_block = (free_block *)((char *)block + required_size);
        new_block->block_size = block_size - required_size;
        new_block->next_free = block->next_free;
        new_block->prev_free = block;
        block->next_free->prev_free = new_block;
        block->next_free = new_block;

        block_footer *new_footer = (block_footer *)((char *)new_block + new_block->block_size - sizeof(block_footer));
        *new_footer = new_block->block_size;
    }
    block->block_size |= 1; // Mark block as allocated
}

static free_block *find_fit(size_t required_size) {
    free_block *head = (free_block *)((char *)mem_heap_lo() + sizeof(size_t));
    for (free_block *block = head->next_free; block != head; block = block->next_free) {
        if (!(block->block_size & 1) && (block->block_size >= required_size)) {
            return block;
        }
    }
    return NULL; // No fit found
}

void *mm_realloc(void *oldptr, size_t size) {
    if (size == 0) {
        mm_free(oldptr);
        return NULL;
    }

    if (oldptr == NULL) {
        return mm_malloc(size);
    }

    allocated_block *old_block = (allocated_block *)((char *)oldptr - sizeof(allocated_block));
    size_t old_size = old_block->block_size - sizeof(allocated_block);

    if (old_size == size) {
        return oldptr; // Size hasn't changed, return the same pointer
    }

    void *newptr = mm_malloc(size);
    if (!newptr) {
        return NULL; // Allocation failed
    }

    size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(newptr, oldptr, copy_size);
    mm_free(oldptr);

    return newptr;
}
