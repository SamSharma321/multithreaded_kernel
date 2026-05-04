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

    uint8_t* dest_char = (uint8_t*)dest;
    uint8_t* src_char = (uint8_t*)src;
    for (int i = 0; i < bytes; i++) {
        dest_char[i] = src_char[i];
    }
    return 0;
}

int memcmp(const void* src, const void* str, int len) {
    if ((src == NULL) || (str == NULL))
        return -EINVARG;

    char* ptr1 = (char*)src;     // byte wise comparison
    char* ptr2 = (char*)str;     // byte wise comparison
    
    while (len-- > 0) {
        if (*(ptr1++) != *(ptr2++))
            return ptr1[-1] < ptr2[-1] ? -1 : 1;
    }
    return 0;
}

