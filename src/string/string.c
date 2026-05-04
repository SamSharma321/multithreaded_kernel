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
