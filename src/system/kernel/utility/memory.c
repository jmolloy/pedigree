int memset(unsigned char *buf, unsigned char c, unsigned int len)
{
  while(len--)
  {
    *buf++ = c;
  }
}

