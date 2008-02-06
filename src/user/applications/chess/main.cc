#include <stdio.h>
#include "Bitboard.h"
#include "Square.h"
#include "moves.h"

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
  
  Bitboard moves = kingMoves(allPieces, enemyPieces, Square(4,4));
  moves.print();
}
