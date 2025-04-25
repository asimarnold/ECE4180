

#ifndef COLLECTION_COMMON_INCLUDED
#define COLLECTION_COMMON_INCLUDED

#include <mbed.h>

//A safer version of malloc, free
//Originally intended to protect against heap corruption from an ISR, but
//due to the operation of new and delete this isn't very effective.
inline void* malloc_safe(size_t size) {
    //__disable_irq();
    void* ret = malloc(size);
    //__enable_irq();
    return ret;
}

inline void free_safe(void* ptr) {
    //__disable_irq();
    free(ptr);
    //__enable_irq();
}

//#define printMalloc(ptr) printfdbg("m: %p %s:%d\n", ptr, __FILE__, __LINE__)

//#define printFree(ptr) printfdbg("f: %p %s:%d\n", ptr, __FILE__, __LINE__)

#define printMalloc(ptr)
#define printFree(ptr)

#endif // COLLECTION_COMMON_INCLUDED