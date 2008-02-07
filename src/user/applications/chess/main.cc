#include <stdio.h>
#include "Bitboard.h"
#include "Square.h"
#include "moves.h"

int main (char argc, char **argv)
{
  initLookupTable();
  unsigned char all[] = {1,0,0,0,0,0,0,0,
			 0,1,0,0,0,0,0,0,
			 0,0,1,0,0,0,0,0,
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
  
  Bitboard moves = rookMoves(allPieces, enemyPieces, Square(1,1));
  moves.print();
// allPieces.rotate180();
// allPieces.print();
}
