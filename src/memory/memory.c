#include "memory.h"


void* memset(void* ptr, int c, size_t size){
    if (ptr != 0) {  // Check for null pointer first before setting it
        char* c_ptr = (char*)ptr;
        for (size_t i = 0; i < size; i++) {
            c_ptr[i] = (char)c;
        }
    }
    return ptr;
}