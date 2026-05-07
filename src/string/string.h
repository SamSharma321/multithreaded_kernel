#ifndef STRING_H_
#define STRING_H_

#include "stdbool.h"

int strlen(const char* str);
int tonumericdigit(const char c);
int strnlen(const char* str, int maxlen);
bool isdigit(const char c);
void strcpy(char* dest, const char* src);
int strncmp(const char* str1, const char* str2, int count);
int strnlen_terminator(const char* str, int max, char terminator);
char tolower(char c);

#endif