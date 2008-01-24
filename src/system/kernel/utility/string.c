#include <stdarg.h>

int strlen(const char *src)
{
  int i = 0;
  while (*src++) i++;
  return i;
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

