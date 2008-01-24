#ifndef KERNEL_UTILITY_H
#define KERNEL_UTILITY_H

#include <stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list arg);
int sprintf(char *buf, const char *fmt, ...);
int strlen(const char *buf);
int memset(unsigned char *buf, unsigned char c, unsigned int len);

#endif

