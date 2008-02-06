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
  king(initialKing)
{
  
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

Square SideState::queen()
{
  nextBoard = queen;
  return next();
}

Square SideState::king()
{
  nextBoard = king;
  return next();
}

Square SideState::next()
{
  return nextBoard.getAndClearFirstSetBit();
}