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

Bitboard rookMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos)
{
  unsigned char rankAll = allPieces.getRank(pos.row);
  unsigned char rankEnemy = enemyPieces.getRank(pos.row);
  unsigned char retRank = rankComparison(rankAll, rankEnemy, pos.col);
  
  Bitboard toRet;
  toRet.setRank(pos.row, retRank);
  
  allPieces.flip180();
  enemyPieces.flip180();
  rankAll = allPieces.getRank(pos.col);
  rankEnemy = enemyPieces.getRank(pos.col);
  retRank = rankComparison(rankAll, rankEnemy, pos.row);
  Bitboard toRet2;
  toRet2.setRank(pos.col, retRank);
  toRet2.flip180();
  
  return toRet | toRet2;
}

Bitboard bishopMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos)
{
  int allPiecesFile, enemyPiecesFile;
  unsigned char rankAll = allPieces.getDiagonalRank45(pos.row, pos.col, &allPiecesFile);
  unsigned char rankEnemy = enemyPieces.getDiagonalRank45(pos.row, pos.col, &enemyPiecesFile);
  unsigned char retRank = rankComparison(rankAll, rankEnemy, allPiecesFile);

  Bitboard toRet;
  toRet.setDiagonalRank45(pos.row, pos.col, retRank);
  
  rankAll = allPieces.getDiagonalRank315(pos.row, pos.col, &allPiecesFile);
  rankEnemy = enemyPieces.getDiagonalRank315(pos.row, pos.col, &enemyPiecesFile);
  retRank = rankComparison(rankAll, rankEnemy, allPiecesFile);
  
  Bitboard toRet2;
  toRet2.setDiagonalRank315(pos.row, pos.col, retRank);
  
  return toRet | toRet2;
}

int main (char argc, char **argv)
{

  unsigned char all[] = {0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,1,0,
			0,0,0,0,0,0,0,1};

  Bitboard allPieces(all);

  unsigned char enemy[] = {0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,0,0,
			  0,0,0,0,0,0,1,0,
			  0,0,0,0,0,0,0,1};
  Bitboard enemyPieces(enemy);
  
  Bitboard moves = bishopMoves(allPieces, enemyPieces, Square(1,1));

  moves.print();
}
