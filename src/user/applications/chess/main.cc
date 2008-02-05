#include <stdio.h>
#include "Bitboard.h"
#include "Square.h"

unsigned char rankComparison(unsigned char allPieces,
			     unsigned char enemyPieces,
			     int file)
{
  // Look right.
  unsigned char toRet = 0;
  
  // Find the first one bit.
  for (int i = file+1; i < 8; i++)
  {
    if ( ((allPieces << i) & 0x80) == 0)
    {
      toRet |= 0x80 >> i;
    }
    else
    {
      // Here we check enemyPieces. If an enemy piece is here then we can
      // add a 1 in the move byte. Else it's a friendly piece and we can't
      // move there.
      if ((enemyPieces << i) & 0x80)
	toRet |= 0x80 >> i;
      break;
    }
  }

  // Look left.
  for (int i = file-1; i >= 0; i--)
  {
    if ( ((allPieces << i) & 0x80) == 0)
      toRet |= 0x80 >> i;
    else
    {
      // Here we check enemyPieces. If an enemy piece is here then we can
      // add a 1 in the move byte. Else it's a friendly piece and we can't
      // move there.
      if ((enemyPieces << i) & 0x80)
	toRet |= 0x80 >> i;
      break;
    }
  }
  return toRet;
}

Bitboard rookMoves(Bitboard allPieces, Bitboard enemyPieces, Square rook)
{

}

int main (char argc, char **argv)
{

  unsigned char all[] = {0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0};

  Bitboard allPieces(all);

  unsigned char enemy[] = {0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0};
  Bitboard enemyPieces(enemy);
}
