#ifndef IO_H_
#define IO_H_

#include <stdint.h>

unsigned char insb(unsigned short port);    // Byte input via port
unsigned short insw(unsigned short port);   // Word input via port

void outb(unsigned short port, unsigned char val);  // Byte output via port
void outw(unsigned short port, unsigned short val); // Word output via port


#endif