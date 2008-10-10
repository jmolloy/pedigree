#ifndef BOARDSTATE_H
#define BOARDSTATE_H

enum Side {White=0, Black=1};

#include "SideState.h"
#include "SearchAlgorithm.h"
#include <stdlib.h>

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
  void move(Move m, Side s, Piece promotion=Pawn);
  
  /**
     Is the given Move by Side s legal?
  **/
  bool isLegal(Move m, Side s);
  
  /**
     Queries
  **/
  bool canCastle(Side side);
  bool inCheck(Side side);
  bool inCheckmate(Side side);
  bool underAttack(Square sq, Side side); // Is (col,row) under attack by side?
  bool rightCastleSquaresFree(Side side);
  bool leftCastleSquaresFree(Side side);

  /**
     Accessors for pieces. Also returns possible moves/attacks in the given bitboard if it is non-NULL.
  **/
  Square firstPawn(Side s, Bitboard *b=NULL);
  Square nextPawn(Side s, Bitboard *b=NULL);
  Square firstRook(Side s, Bitboard *b=NULL);
  Square nextRook(Side s, Bitboard *b=NULL);
  Square firstKnight(Side s, Bitboard *b=NULL);
  Square nextKnight(Side s, Bitboard *b=NULL);
  Square firstBishop(Side s, Bitboard *b=NULL);
  Square nextBishop(Side s, Bitboard *b=NULL);
  Square firstQueen(Side s, Bitboard *b=NULL);
  Square nextQueen(Side s, Bitboard *b=NULL);
  Square firstKing(Side s, Bitboard *b=NULL);
  
  /**
   * Swaps sides.
   */
  void swapSides();
  
  /**
   * Get the static heuristic for this state, for 'side'.
   */
  long heuristic(Side side);

  /**
   * For debug purposes.
   */
  void dump();

  /**
   * Attempts to find this state in the global StateStore.
   */
  bool load(int minPly, Side toPlay, MoveList &ml, long &h);
  
  /**
   * Saves this state into the global StateStore.
   */
  void save(int ply, Side toPlay, long h, MoveList &ml);
  void savePartial();

  void uncache(Side toPlay);

  uint32_t hash();
  uint32_t hash2();

  SideState white;
  SideState black;
  
  bool whiteInCheck; bool whiteCheckStateCalculated;
  bool blackInCheck; bool blackCheckStateCalculated;
  bool whiteInMate;  bool whiteMateStateCalculated;
  bool blackInMate;  bool blackMateStateCalculated;
  long cachedHash;   bool hashCalculated;
  long cachedHash2;  bool hash2Calculated;
};

#endif
