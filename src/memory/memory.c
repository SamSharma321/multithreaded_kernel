#include "memory.h"
#include "status.h"
#include "stdint.h"


void* memset(void* ptr, int c, size_t size){
    if (ptr != 0) {  // Check for null pointer first before setting it
        char* c_ptr = (char*)ptr;
        for (size_t i = 0; i < size; i++) {
            c_ptr[i] = (char)c;
        }
    }
    return ptr;
}

int memcpy(void* dest, void* src, int bytes) {
    if ((!dest) || (!src))
        return -EINVARG;

    char* dest_char = (uint8_t*)dest;
    char* src_char = (uint8_t*)src;
    for (int i = 0; i < bytes; i++) {
        dest_char[i] = src_char[i];
    }
    return 0;
}