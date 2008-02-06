#include "moves.h"

unsigned char rankComparison(unsigned char allPieces,
                             unsigned char enemyPieces,
                             int file)
{
  // Look right.
  unsigned char toRet = 0;
  
  /// 
  /// The following commented out code *does* work, but is x86 specific and I'm not convinced as to
  /// exactly how much speedup it gives...
  ///
  
  
  // Mask off everything but the right hand side.
//   unsigned char masked = allPieces & (0xFF >> (file+1));
//   if (masked == 0)
//   {
//     toRet = 0xFF >> (file+1);
//   }
//   else
//   {
//     unsigned int off;
//     unsigned int tmp = masked;
//     asm volatile("bsr %1, %0" : "=r" (off) : "r" (tmp));
//     // off now holds the bit offset of the last set bit, or the first, scanning from the right.
//     // We want to have a byte with ones between (8-file) and off inclusive (if bit off is set in
//     // enemyPieces), or excluding bit off otherwise.
//     if (!(enemyPieces & (0x1 << off)))
//       off++;
//     unsigned char fin = 0xFF >> file+1;
//     unsigned char fin2 = 0xFF << off;
//     toRet = fin & fin2;
//   }
  //   
//   masked = allPieces & (0xFF << (8-file));
//   if (masked == 0)
//   {
//     toRet |= 0xFF << (8-file);
//     return toRet;
//   }
//   else
//   {
//     unsigned int off;
//     unsigned int tmp = masked;
//     asm volatile("bsf %1, %0" : "=r" (off) : "r" (tmp));
//     // off now holds the bit offset of the first bit set, or the last, scanning from the right.
//     // We want to have a byte with ones between (file-1) and off inclusive (if bit off is set in
//     // enemyPieces), or excluding bit off otherwise.
//     if (!(enemyPieces & (0x1 << off)))
//       off--;
//     unsigned char fin = 0xFF << 8-file;
//     unsigned char fin2 = 0xFF >> off;
//     toRet |= fin & fin2;
//     return toRet;
//   }
  
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
  
  allPieces.flip();
  enemyPieces.flip();
  rankAll = allPieces.getRank(pos.col);
  rankEnemy = enemyPieces.getRank(pos.col);
  retRank = rankComparison(rankAll, rankEnemy, pos.row);
  Bitboard toRet2;
  toRet2.setRank(pos.col, retRank);
  toRet2.flip();
  
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

Bitboard queenMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos)
{
  return rookMoves(allPieces, enemyPieces, pos) | bishopMoves(allPieces, enemyPieces, pos);
}

unsigned char kJumpTable[] = {0,1,0,1,0,0,0,0,
                              1,0,0,0,1,0,0,0,
                              0,0,0,0,0,0,0,0,
                              1,0,0,0,1,0,0,0,
                              0,1,0,1,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0};

Bitboard knightMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos)
{
  Bitboard kTable(kJumpTable);
  kTable.shift(pos.col-2, pos.row-2); // That jump table is centred on (2,2).
  
  // So we now have a bitboard for all the places the knight can move. But we really need to AND that with
  // places that white pieces AREN'T.
  Bitboard noPieces = ~allPieces; // No pieces at all in these squares.
  Bitboard noFriendlyPieces = noPieces | enemyPieces;
  return kTable & noFriendlyPieces;
}

unsigned char KJumpTable[] = {1,1,1,0,0,0,0,0,
                              1,0,1,0,0,0,0,0,
                              1,1,1,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0};
unsigned char leftCastleSquares[] =  {0,1,1,1,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0};
unsigned char rightCastleSquares[] = {0,0,0,0,0,1,1,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0};
unsigned char leftCastleMove   [] =  {1,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0};
unsigned char rightCastleMove   [] = {0,0,0,0,0,0,0,1,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0};

Bitboard kingMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos, bool leftRookMoved,
                   bool rightRookMoved, bool kingMoved)
{
  Bitboard kTable(KJumpTable);
  kTable.shift(pos.col-1, pos.row-1); // That jump table is centred on (1,1).

  // So we now have a bitboard for all the places the knight can move. But we really need to AND that with
  // places that white pieces AREN'T.
  Bitboard noPieces = ~allPieces; // No pieces at all in these squares.
  Bitboard noFriendlyPieces = noPieces | enemyPieces;
  Bitboard toRet = kTable & noFriendlyPieces;
  
  // Now, can we castle?
  if (!kingMoved && (!rightRookMoved || !leftRookMoved))
  {
    if (!rightRookMoved)
    {
      // So, we can castle to the right if and only if the right castle squares are empty.
      if ( (allPieces & rightCastleSquares).empty() )
        toRet = toRet | rightCastleMove;
    }
    if (!leftRookMoved)
    {
      // So, we can castle to the left if and only if the left castle squares are empty.
      if ( (allPieces & leftCastleSquares).empty() )
        toRet = toRet | leftCastleMove;
    }
  }
  return toRet;
}

Bitboard pawnMoves(Bitboard allPieces, Bitboard enemyPieces, Bitboard enemyPawns, Square pos)
{
  
}
