#include "Bitboard.h"

Bitboard::Bitboard()
{
}

Bitboard::Bitboard(unsigned char squares[64])
{
  for (int i = 0; i < 64; i++)
  {
    if (squares[i] != 0)
      set(i/8, i%8);
  }
}
