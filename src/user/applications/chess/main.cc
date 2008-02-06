#include <stdio.h>
#include "Bitboard.h"
#include "Square.h"
#include "moves.h"

int main (char argc, char **argv)
{

  unsigned char all[] = {1,0,0,0,0,0,1,0,
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
  
//   Bitboard moves = kingMoves(allPieces, enemyPieces, Square(1,1), false, false, false);
//   moves.print();
allPieces.rotate180();
allPieces.print();
}
