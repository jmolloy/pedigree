#include "BoardState.h"
#include "moves.h"
#include "SearchAlgorithm.h"
#include "hashes.h"
#include "StateStore.h"

long g_nHashes=0;
long g_nIsLegal = 0;
long g_nInCheck = 0;

BoardState::BoardState() :
  whiteInCheck(false), whiteCheckStateCalculated(false),
  blackInCheck(false), blackCheckStateCalculated(false),
  whiteInMate(false), whiteMateStateCalculated(false),
  blackInMate(false), blackMateStateCalculated(false),
  cachedHash(0), hashCalculated(false),
  cachedHash2(0), hash2Calculated(false)
{
  // Black needs its queen and king swapped around.
  Bitboard tmp = black.queen;
  black.queen = black.king;
  black.king = tmp;
}

BoardState::~BoardState()
{
}

void BoardState::move(Move m, Side s, Piece promotion)
{
  if (s == White)
  {
    Bitboard tmp = white.pawns;
    white.friendlyMove(m, promotion);
    tmp.rotate180();
    Move m2 = m;
    m2.rotate180();
    black.enemyMove(m2, tmp);
  }
  else
  {
    Move m2 = m;
    m2.rotate180();
    Bitboard tmp = black.pawns;
    black.friendlyMove(m2, promotion);
    tmp.rotate180();
    white.enemyMove(m, tmp);
  }
  whiteCheckStateCalculated = whiteMateStateCalculated = blackCheckStateCalculated = blackMateStateCalculated = false;
  hashCalculated = hash2Calculated = false;
}

bool BoardState::isLegal(Move m, Side s)
{
  g_nIsLegal++;
  if (s == White)
  {
    Bitboard b = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
    b.rotate180();
    Bitboard ep = black.enPassant;
    ep.rotate180();
    return white.isLegal(m, b, ep);
  }
  else
  {
    m.rotate180();
    Bitboard b = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
    b.rotate180();
    Bitboard ep = white.enPassant;
    ep.rotate180();
    return black.isLegal(m, b, ep);
  }
}

bool BoardState::inCheck(Side side)
{
  if (side == White)
  {
    if (whiteCheckStateCalculated)
      return whiteInCheck;
    else
    {
      whiteCheckStateCalculated = true;
      g_nInCheck++;
      whiteInCheck = underAttack(white.firstKing(), Black);
      return whiteInCheck;
    }
  }
  else
  {
    if (blackCheckStateCalculated)
      return blackInCheck;
    else
    {
      Square sq = black.firstKing();
      sq.rotate180();
      blackCheckStateCalculated = true;
      g_nInCheck++;
      blackInCheck = underAttack(sq, White);
      return blackInCheck;
    }
  }
}

bool BoardState::inCheckmate(Side side)
{
  bool *result;
  if (side == White && whiteMateStateCalculated)
    return whiteInMate;
  else if (side == Black && blackMateStateCalculated)
    return blackInMate;
  else if (side == White)
  {
    whiteMateStateCalculated = true;
    result = &whiteInMate;
  }
  else
  {
    blackMateStateCalculated = true;
    result = &blackInMate;
  }
  
  // It's false unless we get to the end.
  *result = false;
  
  if (!inCheck(side))
  {
    return false;
  }

  Bitboard b;
  Square sq;
  for (sq = firstPawn(side, &b);
       !sq.invalid();
       sq = nextPawn(side, &b))
  {
    for (Square sq2 = b.getAndClearFirstSetBit();
         !sq2.invalid();
         sq2 = b.getAndClearFirstSetBit())
    {
      MoveList tmpList, tmpList2;
      BoardState newState = *this;
      newState.move( Move(sq.col, sq.row, sq2.col, sq2.row), side, Queen /* Default promotion piece */ );
      if (!newState.inCheck(side))
        return false;
    }
  }
  for (sq = firstKnight(side, &b);
       !sq.invalid();
       sq = nextKnight(side, &b))
  {
    for (Square sq2 = b.getAndClearFirstSetBit();
         !sq2.invalid();
         sq2 = b.getAndClearFirstSetBit())
    {
      MoveList tmpList, tmpList2;
      BoardState newState = *this;
      newState.move( Move(sq.col, sq.row, sq2.col, sq2.row), side, Queen /* Default promotion piece */ );
      if (!newState.inCheck(side))
        return false;
    }
  }
  for (sq = firstRook(side, &b);
       !sq.invalid();
       sq = nextRook(side, &b))
  {
    for (Square sq2 = b.getAndClearFirstSetBit();
         !sq2.invalid();
         sq2 = b.getAndClearFirstSetBit())
    {
      MoveList tmpList, tmpList2;
      BoardState newState = *this;
      newState.move( Move(sq.col, sq.row, sq2.col, sq2.row), side, Queen /* Default promotion piece */ );
      if (!newState.inCheck(side))
        return false;
    }
  }
  for (sq = firstBishop(side, &b);
       !sq.invalid();
       sq = nextBishop(side, &b))
  {
    for (Square sq2 = b.getAndClearFirstSetBit();
         !sq2.invalid();
         sq2 = b.getAndClearFirstSetBit())
    {
      MoveList tmpList, tmpList2;
      BoardState newState = *this;
      newState.move( Move(sq.col, sq.row, sq2.col, sq2.row), side, Queen /* Default promotion piece */ );
      if (!newState.inCheck(side))
        return false;
    }
  }
  for (sq = firstQueen(side, &b);
       !sq.invalid();
       sq = nextQueen(side, &b))
  {
    for (Square sq2 = b.getAndClearFirstSetBit();
         !sq2.invalid();
         sq2 = b.getAndClearFirstSetBit())
    {
      MoveList tmpList, tmpList2;
      BoardState newState = *this;
      newState.move( Move(sq.col, sq.row, sq2.col, sq2.row), side, Queen /* Default promotion piece */ );
      if (!newState.inCheck(side))
        return false;
    }
  }
  sq = firstKing(side, &b);
  for (Square sq2 = b.getAndClearFirstSetBit();
       !sq2.invalid();
       sq2 = b.getAndClearFirstSetBit())
  {
    MoveList tmpList, tmpList2;
    BoardState newState = *this;
    newState.move( Move(sq.col, sq.row, sq2.col, sq2.row), side, Queen /* Default promotion piece */ );
    if (!newState.inCheck(side))
      return false;
  }
  *result = true;
  return true;
}

bool BoardState::underAttack(Square sq, Side side)
{
  if (side == White)
  {
    Bitboard b = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
    b.rotate180();
    return white.isAttacking(sq, b);
  }
  else
  {
    Bitboard b = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
    b.rotate180();
    sq.rotate180();
    return black.isAttacking(sq, b);
  }
}

Square BoardState::firstPawn(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.firstPawn();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      Bitboard enPassant = black.enPassant;
      enPassant.rotate180();
      *b = pawnMoves(allPieces, enemyPieces, enPassant,sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.firstPawn();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      Bitboard enPassant = white.enPassant;
      enPassant.rotate180();
      *b = pawnMoves(allPieces, enemyPieces, enPassant, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::nextPawn(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      Bitboard enPassant = black.enPassant;
      enPassant.rotate180();
      *b = pawnMoves(allPieces, enemyPieces, enPassant,sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      Bitboard enPassant = white.enPassant;
      enPassant.rotate180();
      *b = pawnMoves(allPieces, enemyPieces, enPassant, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::firstRook(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.firstRook();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = rookMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.firstRook();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = rookMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::nextRook(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = rookMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = rookMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::firstKnight(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.firstKnight();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = knightMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.firstKnight();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = knightMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::nextKnight(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = knightMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = knightMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::firstBishop(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.firstBishop();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = bishopMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.firstBishop();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = bishopMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::nextBishop(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = bishopMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = bishopMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::firstQueen(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.firstQueen();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = queenMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.firstQueen();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = queenMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::nextQueen(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = queenMoves(allPieces, enemyPieces, sq);
    }
    return sq;
  }
  else
  {
    Square sq = black.next();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = queenMoves(allPieces, enemyPieces, sq);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

Square BoardState::firstKing(Side s, Bitboard *b)
{
  if (s == White)
  {
    Square sq = white.firstKing();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      allPieces.rotate180(); // Black goes at the bottom.
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      *b = kingMoves(allPieces, enemyPieces, sq, white.rooksMoved[0], white.rooksMoved[1], white.kingMoved);
    }
    return sq;
  }
  else
  {
    Square sq = black.firstKing();
    if (b && !sq.invalid())
    {
      Bitboard allPieces = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
      allPieces.rotate180(); // White goes at the bottom, as it is technically "black" :S
      Bitboard enemyPieces = allPieces;
      allPieces = allPieces | black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
      Square sq2 = sq;
      sq2.rotate180();
      *b = kingMoves(allPieces, enemyPieces, sq, black.rooksMoved[0], black.rooksMoved[1], black.kingMoved);
      b->rotate180(); // Move the bitboard back the right way around.
      return sq2;
    }
    return sq;
  }
}

void BoardState::swapSides()
{
  SideState tmp = white;
  white = black;
  black = tmp;
}

long BoardState::heuristic(Side side)
{
  if (side == White)
    return white.heuristic() - black.heuristic();
  else
    return black.heuristic() - white.heuristic();
}

void BoardState::dump()
{
  printf("+-+-+-+-+-+-+-+-+\n");
  for (int row = 7; row >= 0; row--)
  {
    printf("|");
    for (int col = 0; col < 8; col++)
    {
      Bitboard b, b2;
      b.set(row, col);
      b2 = b;
      b2.rotate180();
           if (b2 & black.pawns)
        printf("P");
      else if (b2 & black.knights)
        printf("H"); // H for 'horse' - K is taken.
      else if (b2 & black.rooks)
        printf("R");
      else if (b2 & black.bishops)
        printf("B");
      else if (b2 & black.queen)
        printf("Q");
      else if (b2 & black.king)
        printf("K");
      else if (b & white.pawns)
        printf("p");
      else if (b & white.knights)
        printf("h"); // H for 'horse' - K is taken.
      else if (b & white.rooks)
        printf("r");
      else if (b & white.bishops)
        printf("b");
      else if (b & white.queen)
        printf("q");
      else if (b & white.king)
        printf("k");
      else
        printf(" ");
      printf("|");
    }
    printf(" %d\n+-+-+-+-+-+-+-+-+\n", row);
  }
  printf(" A B C D E F G H\n");
}

extern StateStore stateStore;

bool BoardState::load(int minPly, Side toPlay, MoveList &ml, long &h)
{
  return stateStore.lookup(this, minPly, toPlay, h, ml);
}

void BoardState::save(int ply, Side toPlay, long h, MoveList &ml)
{
  stateStore.insert(this, toPlay, h, ply, ml);
}

void BoardState::savePartial()
{
  stateStore.insertPartial(this);
}

void BoardState::uncache(Side toPlay)
{
  stateStore.remove(this, toPlay);
}

uint32_t BoardState::hash()
{
  if (hashCalculated)
    return cachedHash;

  hashCalculated = true;
  g_nHashes++;
  uint32_t accum = 0;
  Square sq;
  for (sq = firstPawn(White);
       !sq.invalid();
       sq = nextPawn(White))
  {
    accum ^= hashes[White][0][sq.col][sq.row];
  }
  for (sq = firstKnight(White);
       !sq.invalid();
       sq = nextKnight(White))
  {
    accum ^= hashes[White][1][sq.col][sq.row];
  }
  for (sq = firstRook(White);
       !sq.invalid();
       sq = nextRook(White))
  {
    accum ^= hashes[White][2][sq.col][sq.row];
  }
  for (sq = firstBishop(White);
       !sq.invalid();
       sq = nextBishop(White))
  {
    accum ^= hashes[White][3][sq.col][sq.row];
  }
  for (sq = firstQueen(White);
       !sq.invalid();
       sq = nextQueen(White))
  {
    accum ^= hashes[White][4][sq.col][sq.row];
  }
  sq = firstKing(White);
  accum ^= hashes[White][5][sq.col][sq.row];

  for (sq = firstPawn(Black);
       !sq.invalid();
       sq = nextPawn(Black))
  {
    accum ^= hashes[Black][0][sq.col][sq.row];
  }
  for (sq = firstKnight(Black);
       !sq.invalid();
       sq = nextKnight(Black))
  {
    accum ^= hashes[Black][1][sq.col][sq.row];
  }
  for (sq = firstRook(Black);
       !sq.invalid();
       sq = nextRook(Black))
  {
    accum ^= hashes[Black][2][sq.col][sq.row];
  }
  for (sq = firstBishop(Black);
       !sq.invalid();
       sq = nextBishop(Black))
  {
    accum ^= hashes[Black][3][sq.col][sq.row];
  }
  for (sq = firstQueen(Black);
       !sq.invalid();
       sq = nextQueen(Black))
  {
    accum ^= hashes[Black][4][sq.col][sq.row];
  }
  sq = firstKing(Black);
  accum ^= hashes[Black][5][sq.col][sq.row];

  if (white.rooksMoved[0])
    accum ^= hashWhiteRookMovedL;
  if (white.rooksMoved[1])
    accum ^= hashWhiteRookMovedR;
  if (white.kingMoved)
    accum ^= hashWhiteKingMoved;
  if (black.rooksMoved[0])
    accum ^= hashBlackRookMovedL;
  if (black.rooksMoved[1])
    accum ^= hashBlackRookMovedR;
  if (black.kingMoved)
    accum ^= hashBlackKingMoved;

  sq = white.enPassant.getAndClearFirstSetBit();
  if (!sq.invalid())
    accum ^= hashes[White][6 /*En Passant*/][sq.col][sq.row];
  sq = black.enPassant.getAndClearFirstSetBit();
  if (!sq.invalid())
    accum ^= hashes[Black][6 /*En Passant*/][sq.col][sq.row];

  cachedHash = accum;
  return accum;
}

uint32_t BoardState::hash2()
{
  if (hash2Calculated)
    return cachedHash2;
  
  hash2Calculated = true;
  uint32_t accum = 0;
  Square sq;
  for (sq = firstPawn(White);
       !sq.invalid();
       sq = nextPawn(White))
  {
    accum ^= hashes2[White][0][sq.col][sq.row];
  }
  for (sq = firstKnight(White);
       !sq.invalid();
       sq = nextKnight(White))
  {
    accum ^= hashes2[White][1][sq.col][sq.row];
  }
  for (sq = firstRook(White);
       !sq.invalid();
       sq = nextRook(White))
  {
    accum ^= hashes2[White][2][sq.col][sq.row];
  }
  for (sq = firstBishop(White);
       !sq.invalid();
       sq = nextBishop(White))
  {
    accum ^= hashes2[White][3][sq.col][sq.row];
  }
  for (sq = firstQueen(White);
       !sq.invalid();
       sq = nextQueen(White))
  {
    accum ^= hashes2[White][4][sq.col][sq.row];
  }
  sq = firstKing(White);
  accum ^= hashes2[White][5][sq.col][sq.row];

  for (sq = firstPawn(Black);
       !sq.invalid();
       sq = nextPawn(Black))
  {
    accum ^= hashes2[Black][0][sq.col][sq.row];
  }
  for (sq = firstKnight(Black);
       !sq.invalid();
       sq = nextKnight(Black))
  {
    accum ^= hashes2[Black][1][sq.col][sq.row];
  }
  for (sq = firstRook(Black);
       !sq.invalid();
       sq = nextRook(Black))
  {
    accum ^= hashes2[Black][2][sq.col][sq.row];
  }
  for (sq = firstBishop(Black);
       !sq.invalid();
       sq = nextBishop(Black))
  {
    accum ^= hashes2[Black][3][sq.col][sq.row];
  }
  for (sq = firstQueen(Black);
       !sq.invalid();
       sq = nextQueen(Black))
  {
    accum ^= hashes2[Black][4][sq.col][sq.row];
  }
  sq = firstKing(Black);
  accum ^= hashes2[Black][5][sq.col][sq.row];
  
  if (white.rooksMoved[0])
    accum ^= hashWhiteRookMovedL2;
  if (white.rooksMoved[1])
    accum ^= hashWhiteRookMovedR2;
  if (white.kingMoved)
    accum ^= hashWhiteKingMoved2;
  if (black.rooksMoved[0])
    accum ^= hashBlackRookMovedL2;
  if (black.rooksMoved[1])
    accum ^= hashBlackRookMovedR2;
  if (black.kingMoved)
    accum ^= hashBlackKingMoved2;

  sq = white.enPassant.getAndClearFirstSetBit();
  if (!sq.invalid())
    accum ^= hashes2[White][6 /*En Passant*/][sq.col][sq.row];
  sq = black.enPassant.getAndClearFirstSetBit();
  if (!sq.invalid())
    accum ^= hashes2[Black][6 /*En Passant*/][sq.col][sq.row];
  
  cachedHash2 = accum;
  return accum;
}
