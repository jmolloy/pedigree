#include "SideState.h"

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
  kingMoved(false),
  nextBoard()
{
  rooksMoved[0] = false;
  rooksMoved[1] = false;
}

Square SideState::firstPawn()
{
  nextBoard = pawns;
  return next();
}

Square SideState::firstRook()
{
  nextBoard = rooks;
  return next();
}

Square SideState::firstKnight()
{
  nextBoard = knights;
  return next();
}

Square SideState::firstBishop()
{
  nextBoard = bishops;
  return next();
}

Square SideState::firstQueen()
{
  nextBoard = queen;
  return next();
}

Square SideState::firstKing()
{
  nextBoard = king;
  return next();
}

Square SideState::next()
{
  return nextBoard.getAndClearFirstSetBit();
}

bool SideState::inCheck()
{
  return underAttack(firstKing());
}

bool SideState::underAttack(Square sq)
{
}

bool SideState::isCastle(Move m)
{
  Square king = firstKing();
  return (king.col == m.col1 && king.row == m.row2 &&
          (king.col2-king.col1 > 1 || king.col2-king.col1 < -1))
}

void SideState::move(Move m)
{

}
