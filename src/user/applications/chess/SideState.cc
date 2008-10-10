#include "SideState.h"
#include "moves.h"

long g_nIsAttacking = 0;
long g_nHeuristic = 0;

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
  hasCastled(false),
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
  g_nIsAttacking++;
  // Make a bitboard for the square under attack.
  Bitboard sqBoard;
  sqBoard.set(sq.row, sq.col);
  
  // Make a bitboard for all friendly pieces.
  Bitboard frBoard = pawns | rooks | knights | bishops | king | queen;
  
  // And a bit board for all pieces.
  Bitboard all = enemyPieces | frBoard;
  for (Square i = firstPawn();
       !i.invalid();
       i = next())
  {
    // NOTE: ignores En Passant atm!
    if ( pawnAttackOnly(all, enemyPieces, Bitboard(), i) & sqBoard )
      return true;
  }
  for (Square i = firstRook();
       !i.invalid();
       i = next())
  {
    if ( rookMoves(all, enemyPieces, i) & sqBoard )
      return true;
  }
  for (Square i = firstBishop();
       !i.invalid();
       i = next())
  {
    if ( bishopMoves(all, enemyPieces, i) & sqBoard )
      return true;
  }
  for (Square i = firstQueen();
       !i.invalid();
       i = next())
  {
    if ( queenMoves(all, enemyPieces, i) & sqBoard )
      return true;
  }
  if (!firstKing().invalid() && 
      (kingMoves (all, enemyPieces, firstKing(), true, true, true)) & sqBoard) // Can't attack with a castle!
    return true;

  return false;
}

bool SideState::isCastle(Move m)
{
  Square k = firstKing();
  return (k.col == m.sCol && k.row == m.eRow &&
          (m.eCol-m.sCol > 1 || m.eCol-m.sCol < -1));
}

bool SideState::isLegal(Move m, Bitboard enemyPieces, Bitboard enemyPawnsEnPassant)
{
  Square startSq(m.sCol, m.sRow);
  Square endSq(m.eCol, m.eRow);

  Bitboard allPieces = pawns|knights|rooks|bishops|queen|king|enemyPieces;
  if (pawns & Bitboard(startSq))
  {
    // Pawn moving.
    if ( pawnMoves(allPieces, enemyPieces, enemyPawnsEnPassant, startSq) & Bitboard(endSq) )
      return true;
  }
  else if (knights & Bitboard(startSq))
  {
    // Knight moving.
    if ( knightMoves(allPieces, enemyPieces, startSq) & Bitboard(endSq) )
      return true;
  }
  else if (rooks & Bitboard(startSq))
  {
    // Rook moving.
    if ( rookMoves(allPieces, enemyPieces, startSq) & Bitboard(endSq) )
      return true;
  }
  else if (bishops & Bitboard(startSq))
  {
    // Bishop moving.
    if ( bishopMoves(allPieces, enemyPieces, startSq) & Bitboard(endSq) )
      return true;
  }
  else if (queen & Bitboard(startSq))
  {
    // Queen moving.
    if ( queenMoves(allPieces, enemyPieces, startSq) & Bitboard(endSq) )
      return true;
  }
  else if (king & Bitboard(startSq))
  {
    // King moving.
    if ( kingMoves(allPieces, enemyPieces, startSq, rooksMoved[0], rooksMoved[1], kingMoved) & Bitboard(endSq) )
      return true;
  }
  return false;
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
    switch (promotion)
    {
    case Pawn:   pawns.set(m.eRow, m.eCol); break;
    case Knight: knights.set(m.eRow, m.eCol); break;
    case Bishop: bishops.set(m.eRow, m.eCol); break;
    case Queen:  queen.set(m.eRow, m.eCol); break;
    case Rook:   rooks.set(m.eRow, m.eCol); break;
    default: printf("Oh fuck.\n");
    }
    operationCompleted = true;
  }
  else if ( (pawns & mb) && ( m.eRow - m.sRow == 2 ) )
  {
    // Pawn moved two spaces. We must set the square that this pawn moved over
    // as "en-passant-able".
    enPassant.set(m.sRow+1, m.sCol);
  }
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
    kingMoved = true; // Doesn't matter about rooksMoved now.
    hasCastled = true;
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
       (enPassant & mb2) )   // To an en passant square?
  {
    // Behold, en passant.
    pawns.clear(m.eRow+1, m.eCol); // Remove the pawn.
    operationCompleted = true;
  }

  // Clear en-passant bitboard.
  enPassant.clear();

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

extern unsigned char pawnAttack[];
unsigned char knightCentre[]={0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              1,1,1,1,1,1,1,1,
                              1,1,1,1,1,1,1,1,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0};

long SideState::heuristic()
{
  long h = 0;
  g_nHeuristic ++;
  ///////////////////////////////////////////////////////////
  // Pawns
  // +100 for each, and +10 for every friendly piece protected by a pawn.
  Bitboard _pawns = pawns;
  for (Square sq = _pawns.getAndClearFirstSetBit();
       !sq.invalid();
       sq = _pawns.getAndClearFirstSetBit())
  {
    h += 100; // +100 for each.
    Bitboard attack(pawnAttack);
    attack.shift(sq.col-1, sq.row); // That jump table is centred on (1,0).
    // What pieces can we defend? (not including king).
    Bitboard finalAttack = attack & (pawns|knights|rooks|bishops|queen);
    while (!finalAttack.getAndClearFirstSetBit().invalid())
      h += 10; // +1 for each friendly piece protected.
  }
  
  ///////////////////////////////////////////////////////////
  // Rooks
  // +400 for each.
  Bitboard _rooks = rooks;
  for (Square sq = _rooks.getAndClearFirstSetBit();
       !sq.invalid();
       sq = _rooks.getAndClearFirstSetBit())
  {
    h += 400; // +400 for each.
  }

  ///////////////////////////////////////////////////////////
  // Knights
  // +500 for each, and +20 for each in the middle set of squares.
  Bitboard _knights = knights;
  for (Square sq = _knights.getAndClearFirstSetBit();
       !sq.invalid();
       sq = _knights.getAndClearFirstSetBit())
  {
    h += 500; // +500 for each.
    Bitboard knightsInCentre = knights & Bitboard(knightCentre);
    while (!knightsInCentre.getAndClearFirstSetBit().invalid())
      h += 20; // +20 for each knight in centre.
  }

  ///////////////////////////////////////////////////////////
  // Bishops
  // +400 for each
  Bitboard _bishops = bishops;
  for (Square sq = _bishops.getAndClearFirstSetBit();
       !sq.invalid();
       sq = _bishops.getAndClearFirstSetBit())
  {
    h += 400; // +400 for each.
  }
  
  ///////////////////////////////////////////////////////////
  // Queen
  // +1000 for each
  Bitboard _queens = queen;
  for (Square sq = _queens.getAndClearFirstSetBit();
       !sq.invalid();
       sq = _queens.getAndClearFirstSetBit())
  {
    h += 1000; // +1000 for each.
  }
  
  ///////////////////////////////////////////////////////////
  // King
  // +50 for castling.
  if (hasCastled)
    h += 50;
  
  return h;
}
