#include <stdio.h>
#include <stdbool.h>

#ifdef DRIVER

extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);
extern void *mm_calloc (size_t nmemb, size_t size);

#else

extern void *malloc (size_t size);
extern void free (void *ptr);
extern void *realloc(void *ptr, size_t size);
extern void *calloc (size_t nmemb, size_t size);

#endif

extern bool mm_init(void);

extern bool mm_checkheap(int lineno);