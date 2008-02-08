#ifndef BOARDSTATE_H
#define BOARDSTATE_H

#include "SideState.h"

enum Side {White=0, Black=1};

/**
   Stores the state of the board.
**/
class BoardState
{
public:
  /**
     Constructs a new BoardState. This defaults to the starting position.
  **/
  BoardState();

  ~BoardState();

  /**
     Performs a Move m, by Side s.
  **/
  void move(Move m, Side s);
  
  /**
   * Is the given Move by Side s legal?
   */
  void isLegal(Move m, Side s);
  
  /**
     Queries
  **/
  bool canCastle(Side side);
  bool inCheck(Side side);
  bool underAttack(int col, int row, Side side); // Is (col,row) under attack by side?
  bool rightCastleSquaresFree(Side side);
  bool leftCastleSquaresFree(Side side);

  /**
     Accessors for pieces. Also returns possible moves/attacks in the given bitboard if it is non-NULL.
  **/
  Square firstPawn(Side s, Bitboard *b=NULL);
  Square firstRook(Side s, Bitboard *b=NULL);
  Square firstKnight(Side s, Bitboard *b=NULL);
  Square firstBishop(Side s, Bitboard *b=NULL);
  Square firstQueen(Side s, Bitboard *b=NULL);
  Square firstKing(Side s, Bitboard *b=NULL);
  Square next(Side s, Bitboard *b=NULL);
  
  /**
   * Swaps sides.
   */
  void swapSides();

private:

  SideState white;
  SideState black;

};

#endif
