#ifndef SIDESTATE_H
#define SIDESTATE_H

#include "Bitboard.h"
#include "Move.h"
#include "Square.h"

/**
   A representation of a side (white or black). The representation
   of each side is the same - each thinks he/she is White (bottom of the
   board). The BoardState object rotates queries, moves and responses depending
   on whether this side is actually white or black.
**/
class SideState
{
public:
  /**
     Constructor. Starts the state in the normal starting position.
  **/
  SideState();

  ~SideState() {}

  /**
     Lets a move be made, regardless of legality.
  **/
  void move(Move m);

  /**
   * Evaluates to true if the given Move is legal.
   */
  bool isLegal(Move m);
  
  /**
     Evaluates to true if the given Move is a castle.
  **/
  bool isCastle(Move m);
  
  /**
     Queries.
  **/
  bool inCheck();
  bool underAttack(Square sq); // Is (col,row) under attack by side?
  // TODO add en passant.
  
  /**
     Accessors for pieces.
  **/
  Square firstPawn();
  Square firstRook();
  Square firstKnight();
  Square firstBishop();
  Square firstQueen();
  Square firstKing();
  
  Square next();
  
  /**
     General state.
  **/
  bool rooksMoved[2];
  bool kingMoved;

  /**
     Bitboards.
  **/
  Bitboard pawns;
  Bitboard rooks;
  Bitboard bishops;
  Bitboard knights;
  Bitboard queen;
  Bitboard king;

  /**
   * Bitboard for the next() function.
   */
  Bitboard nextBoard;
  
};

#endif
