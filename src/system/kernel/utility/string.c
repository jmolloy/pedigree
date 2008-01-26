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

int strcmp(char *p1, char *p2)
{
  int i = 0;
  int failed = 0;
  while(p1[i] != '\0' && p2[i] != '\0')
  {
    if(p1[i] != p2[i])
    {
      failed = 1;
      break;
    }
    i++;
  }
  // why did the loop exit?
  if( (p1[i] == '\0' && p2[i] != '\0') || (p1[i] != '\0' && p2[i] == '\0') )
    failed = 1;

  return failed;
}

int strncmp(char *p1, char *p2, int n)
{
  int i = 0;
  int failed = 0;
  while(p1[i] != '\0' && p2[i] != '\0' && n)
  {
    if(p1[i] != p2[i])
    {
      failed = 1;
      break;
    }
    i++;
    n--;
  }
  // why did the loop exit?
  if( n && ( (p1[i] == '\0' && p2[i] != '\0') || (p1[i] != '\0' && p2[i] == '\0') ) )
    failed = 1;

  return failed;
}


char *strcat(char *dest, const char *src)
{
  int di = strlen(dest);
  int si = 0;
  while (src[si])
    dest[di++] = src[si++];
  
  dest[di] = '\0';
}
