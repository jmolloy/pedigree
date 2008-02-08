#include "SideState.h"
#include "moves.h"

unsigned char initialPawns[] = {0,0,0,0,0,0,0,0,
                                1,1,1,1,1,1,1,1,
                                0,0,0,0,0,0,0,0,
                                0,0,0,0,0,0,0,0,
                                0,0,0,0,0,0,0,0,
                                0,0,0,0,0,0,0,0,
                                0,0,0,0,0,0,0,0,
                                0,0,0,0,0,0,0,0};
unsigned char initialKnights[] = {0,1,0,0,0,0,1,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
unsigned char initialRooks[] =   {1,0,0,0,0,0,0,1,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
unsigned char initialBishops[] = {0,0,1,0,0,1,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
unsigned char initialQueen[] =   {0,0,0,1,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
unsigned char initialKing[] =    {0,0,0,0,1,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};

SideState::SideState() :
  pawns(initialPawns),
  rooks(initialRooks),
  knights(initialKnights),
  bishops(initialBishops),
  queen(initialQueen),
  king(initialKing),
  enPassant(),
  kingMoved(false),
  nextBoard()
{
  rooksMoved[0] = false;
  rooksMoved[1] = false;
}

Square SideState::firstPawn()
{
  nextBoard = pawns;
  nextPiece = Pawn;
  return next();
}

Square SideState::firstRook()
{
  nextBoard = rooks;
  nextPiece = Rook;
  return next();
}

Square SideState::firstKnight()
{
  nextBoard = knights;
  nextPiece = Knight;
  return next();
}

Square SideState::firstBishop()
{
  nextBoard = bishops;
  nextPiece = Bishop;
  return next();
}

Square SideState::firstQueen()
{
  nextBoard = queen;
  nextPiece = Queen;
  return next();
}

Square SideState::firstKing()
{
  nextBoard = king;
  nextPiece = King;
  return next();
}

Square SideState::next()
{
  return nextBoard.getAndClearFirstSetBit();
}

bool SideState::isAttacking(Square sq, Bitboard enemyPieces)
{
  // Make a bitboard for the square under attack.
  Bitboard sqBoard;
  sqBoard.set(sq.row, sq.col);
  
  // Make a bitboard for all friendly pieces.
  Bitboard frBoard = pawns | rooks | knights | bishops | king | queen;
  
  // And a bit board for all pieces.
  Bitboard all = enemyPieces | frBoard;
  
  for (Square i = firstPawn();
       i.col != (unsigned char)-1;
       i = next())
  {
    // NOTE: ignores En Passant atm!
    if ( pawnMoves(all, enemyPieces, Bitboard(), sq) & sqBoard )
      return true;
  }
  for (Square i = firstRook();
       i.col != (unsigned char)-1;
       i = next())
  {
    if ( rookMoves(all, enemyPieces, sq) & sqBoard )
      return true;
  }
  for (Square i = firstBishop();
       i.col != (unsigned char)-1;
       i = next())
  {
    if ( bishopMoves(all, enemyPieces, sq) & sqBoard )
      return true;
  }
  if (firstQueen().col != (unsigned char)-1 &&
      (queenMoves (all, enemyPieces, sq) & sqBoard))
    return true;
  if (firstKing().col != (unsigned char)-1 && 
      (kingMoves (all, enemyPieces, sq, true, true, true))) // Can't attack with a castle!
    return true;

  return false;
}

bool SideState::isCastle(Move m)
{
  Square k = firstKing();
  return (k.col == m.sCol && k.row == m.eRow &&
          (m.eCol-m.sCol > 1 || m.eCol-m.sCol < -1));
}

bool SideState::isLegal(Move m)
{

}

void SideState::friendlyMove(Move m, Piece promotion)
{
  Bitboard mb, mb2;
  mb.set(m.sRow, m.sCol);
  mb2.set(m.eRow, m.eCol);
  bool operationCompleted = false;

  // We must identify if this move is a pawn, or a king, as special things can happen
  // (promotion, castle).
  if ( (pawns & mb) && (m.eRow == 7)) // Pawn promotion
  {
    pawns = pawns & ~mb; // Remove the pawn.
//     addPiece(promotion, Square(m.eRow, m.eCol) );
    operationCompleted = true;
  } //TODO pawn 2 piece move, then other move to clear en passant
  else if ( (king & mb) && ( (m.sCol - m.eCol > 1) || (m.eCol - m.sCol > 1)) )
  {
    // Castle ahoy!
    king = Bitboard();
    king.set(m.eRow, m.eCol);
    
    // Castling left?
    if (m.eCol == 2)
    {
      // Remove left rook.
      rooks.clear(0, 0);
      // Add it again.
      rooks.set(0, 3);
    }
    else
    {
      // Remove right rook.
      rooks.clear(0, 7);
      // Add it again.
      rooks.set(0, 5);
    }
    operationCompleted = true;
  }
  
  if (!operationCompleted)
  {
    if (pawns & mb)
    {
      pawns = pawns & ~mb;
      pawns = pawns | mb2;
    }
    if (knights & mb)
    {
      knights = knights & ~mb;
      knights = knights | mb2;
    }
    if (rooks & mb)
    {
      rooks = rooks & ~mb;
      rooks = rooks | mb2;
    }
    if (bishops & mb)
    {
      bishops = bishops & ~mb;
      bishops = bishops | mb2;
    }
    if (queen & mb)
    {
      queen = queen & ~mb;
      queen = queen | mb2;
    }
    if (king & mb)
    {
      king = king & ~mb;
      king = king | mb2;
    }
  }
}

void SideState::enemyMove(Move m, Bitboard enemyPawns)
{
  // Check for en passant.
  Bitboard mb, mb2;
  mb.set(m.sRow, m.sCol);
  mb2.set(m.eRow, m.eCol);
  Bitboard nmb2 = ~mb2;
  bool operationCompleted = false;
  
  if ( (enemyPawns & mb) &&  // A pawn moving?
       (m.eRow == 2) &&      // To row 2?
       (m.sCol != m.eCol) && // Diagonally?
       !(pawns & mb2 ) )     // Without there being one of my pawns there?
  {
    // Behold, en passant.
    pawns.clear(m.eRow+1, m.eCol); // Remove the pawn.
    operationCompleted = true;
  }
  
  if (!operationCompleted)
  {
    pawns = pawns & nmb2;
    rooks = rooks & nmb2;
    bishops = bishops & nmb2;
    knights = knights & nmb2;
    queen = queen & nmb2;
    king = king & nmb2;
  }
}
