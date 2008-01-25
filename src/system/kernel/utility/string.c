#include <stdarg.h>

int strlen(const char *src)
{
  int i = 0;
  while (*src++) i++;
  return i;
}

int strcpy(char *dest, const char *src)
{
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
}

int strncpy(char *dest, const char *src, int len)
{
  while (*src && len)
  {
    *dest++ = *src++;
    len--;
  }
  *dest = '\0';
}

int sprintf(char *buf, const char *fmt, ...)
{
  va_list args;
  int i;

  va_start(args, fmt);
  i = vsprintf(buf, fmt, args);
  va_end(args);

  return i;
}

