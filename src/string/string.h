#ifndef STRING_H_
#define STRING_H_

#include "stdbool.h"

int strlen(const char* str);
int tonumericdigit(const char c);
int strnlen(const char* str, int maxlen);
bool isdigit(const char c);

#endif