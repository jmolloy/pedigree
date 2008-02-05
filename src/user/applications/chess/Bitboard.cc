#include "Bitboard.h"
#include <stdio.h>
#include <string.h>

Bitboard::Bitboard()
{
  member.i = 0;
}

Bitboard::Bitboard(unsigned char squares[64])
{
  member.i = 0;
  for (int i = 0; i < 64; i++)
  {
    if (squares[i] != 0)
      set(i/8, i%8);
  }
}

void Bitboard::print()
{
  for (int i = 7; i >= 0; i--)
  {
    for (int j = 0; j < 8; j++)
    {
      if (test(i,j))
        putchar ('x');
      else
        putchar ('.');
    }
    printf(" %d\n", i+1);
  }
  printf("ABCDEFGH\n");
}

void Bitboard::flip180()
{
  // Rows become columns, columns become rows.
  char c[8];
  memset(c, 0, 8);
  
  for(int i = 0; i < 8 ; i++)
  {
    c[0] |= (member.c[i] & 0x80) >> i;
    c[1] |= (i-1 < 0) ? (member.c[i] & 0x40) << 1-i : (member.c[i] & 0x40) >> i-1;
    c[2] |= (i-2 < 0) ? (member.c[i] & 0x20) << 2-i : (member.c[i] & 0x20) >> i-2;
    c[3] |= (i-3 < 0) ? (member.c[i] & 0x10) << 3-i : (member.c[i] & 0x10) >> i-3;
    c[4] |= (i-4 < 0) ? (member.c[i] & 0x08) << 4-i : (member.c[i] & 0x08) >> i-4;
    c[5] |= (i-5 < 0) ? (member.c[i] & 0x04) << 5-i : (member.c[i] & 0x04) >> i-5;
    c[6] |= (i-6 < 0) ? (member.c[i] & 0x02) << 6-i : (member.c[i] & 0x02) >> i-6;
    c[7] |= (i-7 < 0) ? (member.c[i] & 0x01) << 7-i : (member.c[i] & 0x01) >> i-7;
  }

  memcpy(member.c, c, 8);
}

unsigned char Bitboard::getDiagonalRank45(int rank, int file, int *newFile)
{
  int f = file;
  int r = rank;
  int nf = 0;
  while (f-1 >= 0 && r-1 >= 0)
  {
    f --;
    r --;
    nf ++;
  }
  
  unsigned char toRet = 0;
  int n = 0;
  while (f < 8 && r < 8)
  {
    if (test(r,f))
      toRet |= 0x80 >> n;
    n++;
    f++;
    r++;
  }
  *newFile = nf;
  return toRet;
}

unsigned char Bitboard::getDiagonalRank315(int rank, int file, int *newFile)
{
  int f = file;
  int r = rank;
  int nf = 0;
  while (f-1 >= 0 && r+1 < 8)
  {
    f --;
    r ++;
    nf ++;
  }
  
  unsigned char toRet = 0;
  int n = 0;
  while (f < 8 && r >= 0)
  {
    if (test(r,f))
      toRet |= 0x80 >> n;
    n++;
    f++;
    r--;
  }
  *newFile = nf;
  return toRet;
}

void Bitboard::setDiagonalRank45(int rank, int file, unsigned char newRank)
{
  int f = file;
  int r = rank;
  while (f-1 >= 0 && r-1 >= 0)
  {
    f --;
    r --;
  }
  
  int n = 0;
  while (f < 8 && r < 8)
  {
    if ( newRank & (0x80 >> n) )
      set (r, f);
    n++;
    f++;
    r++;
  }
}

void Bitboard::setDiagonalRank315(int rank, int file, unsigned char newRank)
{
  int f = file;
  int r = rank;
  while (f-1 >= 0 && r+1 < 8)
  {
    f --;
    r ++;
  }

  int n = 0;
  while (f < 8 && r >= 0)
  {
    if ( newRank & (0x80 >> n) )
      set (r, f);
    n++;
    f++;
    r--;
  }
}

Bitboard Bitboard::operator|(Bitboard &b2)
{
  Bitboard b3;
  b3.member.i = member.i | b2.member.i;
  return b3;
}
