
#include <Bitboard.h>
#include <Move.h>
#include <Square.h>

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
     Lets a move be made.
  **/
  move(Move m);

  /**
     Queries.
  **/
  bool canCastle();
  bool inCheck();
  bool underAttack(Square sq); // Is (col,row) under attack by side?
  bool rightCastleSquaresFree();
  bool leftCastleSquaresFree();

  /**
     Accessors for pieces.
  **/
  Square firstPawn();
  Square nextPawn();

  Square firstRook();
  Square nextRook();

  Square firstKnight();
  Square nextKnight();

  Square firstBishop();
  Square nextBishop();

  Square queen();
  Square king();

private:
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

}
