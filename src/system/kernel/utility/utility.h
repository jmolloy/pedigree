#ifndef KERNEL_UTILITY_H
#define KERNEL_UTILITY_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int vsprintf(char *buf, const char *fmt, va_list arg);
int sprintf(char *buf, const char *fmt, ...);
int strlen(const char *buf);
int strcpy(char *dest, const char *src);
int strncpy(char *dest, const char *src, int len);
int memset(unsigned char *buf, unsigned char c, unsigned int len);
void memcpy(unsigned char *dest, unsigned char *src, unsigned int len);

int strcmp(char *p1, char *p2);
int strncmp(char *p1, char *p2, int n);
char *strcat(char *dest, const char *src);

#ifdef __cplusplus
}
#endif

#endif

