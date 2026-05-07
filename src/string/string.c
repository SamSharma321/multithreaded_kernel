#include "string.h"
#include "status.h"
#include "stddef.h"
#include "kernel.h"

#define REALISTIC_STRLEN    1000

int strlen(const char* str) {
    if (str == NULL)
        return -EINVARG;

    int len = 0;

    while (*(str++) != '\0') {
        len++;
        if (len > REALISTIC_STRLEN) {
            return -ENOMEM;
        }
    }

    return len;
}

int strnlen(const char* str, int maxlen) {
    if ((str == NULL) || (maxlen < 0))
        return -EINVARG;

    int len = strlen(str);

    if ((len > maxlen) || (len == -ENOMEM))
        return maxlen;

    return len;
}

bool isdigit(const char c) {
    return !((c < '0') || (c > '9'));
}

int tonumericdigit(const char c) {
    if (!isdigit(c)) {
        return -EINVARG;
    }

    return (int)(c - '0');
}

void strcpy(char* dest, const char* src) {
    while (*(src)) {
        *(dest++) = *(src++);
    }
    *dest = '\0'; // Append string end literal
}

int strncmp(const char* str1, const char* str2, int count) {
    unsigned char u1, u2;
    while (count-- > 0) {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if (u1 != u2)
            return u1 - u2;
        if (u1 == '\0') 
            return 0;
    }
    return 0;
}

int strnlen_terminator(const char* str, int max, char terminator) {
    int len = 0;
    while ((*str != '\0') && (*str != terminator)) {
        len++;
        str++;
        if (len == max)
            break;
    }
    return len;
}

char tolower(char c) {
    if (c >= 65 && c <= 90) {
        c += 32;
    }
    return c;
}

int istrncmp(const char* s1, const char* s2, int n) {
    unsigned char u1, u2;

    while (n-- > 0) {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if (u1 != u2 && tolower(u1) != tolower(u2)) {
            return u1 - u2;
        }
        if (u1 == '\0')
            return 0;
    }
    return 0;
}


