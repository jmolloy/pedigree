int memset(unsigned char *buf, unsigned char c, unsigned int len)
{
  while(len--)
  {
    *buf++ = c;
  }
}

void memcpy(unsigned char *dest, unsigned char *src, unsigned int len)
{
  const unsigned char *sp = (const unsigned char *)src;
  unsigned char *dp = (unsigned char *)dest;
  for (; len != 0; len--) *dp++ = *sp++;
}

