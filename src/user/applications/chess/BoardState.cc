#include "BoardState.h"


BoardState::BoardState()
{
}

BoardState::~BoardState()
{
}

void BoardState::move(Move m, Side s, Piece promotion=Pawn)
{
  if (s == White)
  {
    white.friendlyMove(m, promotion);
    Bitboard tmp = white.pawns;
    tmp.rotate180();
    black.enemyMove(m, tmp);
  }
  else
  {
    black.friendlyMove(m, promotion);
    Bitboard tmp = black.pawns;
    tmp.rotate180();
    white.enemyMove(m, tmp);
  }
}

void BoardState::isLegal(Move m, Side s)
{
  if (s == White)
    return white.isLegal(m);
  else
    return black.isLegal(m);
}

bool BoardState::inCheck(Side side)
{
  
  if (side == White)
    return underAttack(white.firstKing(), Black);
  else
    return underAttack(black.firstKing(), White);
}

bool BoardState::underAttack(Square sq, Side side)
{
  if (side == White)
  {
    Bitboard b = black.pawns|black.knights|black.rooks|black.bishops|black.queen|black.king;
    b.rotate180()
    return white.isAttacking(sq, b);
  }
  else
  {
    Bitboard b = white.pawns|white.knights|white.rooks|white.bishops|white.queen|white.king;
    b.rotate180();
    sq.rotate180()
    return black.isAttacking(sq2, b);
  }
}

Square BoardState::firstPawn(Side s, Bitboard *b=NULL)
{
  if (s == White)
  {
    Square sq = white.firstPawn();
    if (b)
      *b = pawnMoves(
  }
  else
  {
  }
}
Square BoardState::firstRook(Side s, Bitboard *b=NULL)
{
}
Square BoardState::firstKnight(Side s, Bitboard *b=NULL)
{
}
Square BoardState::firstBishop(Side s, Bitboard *b=NULL)
{
}
Square BoardState::firstQueen(Side s, Bitboard *b=NULL)
{
}
Square BoardState::firstKing(Side s, Bitboard *b=NULL)
{
}
Square BoardState::next(Side s, Bitboard *b=NULL)
{
}

void BoardState::swapSides()
{
  SideState tmp = white;
  white = black;
  black = tmp;
}
