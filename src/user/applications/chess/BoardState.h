
#include <SideState.h>

enum Side {eWhite=0, eBlack=1};

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
     Performs a move.
  **/
  void move(Move m);

  /**
     Queries
  **/
  bool canCastle(Side side);
  bool inCheck(Side side);
  bool underAttack(int col, int row, Side side); // Is (col,row) under attack by side?
  bool rightCastleSquaresFree(Side side);
  bool leftCastleSquaresFree(Side side);

  /**
     Accessors for pieces.
  **/
  Square firstPawn(Side s);
  Square nextPawn(Side s);

  Square firstRook(Side s);
  Square nextRook(Side s);

  Square firstKnight(Side s);
  Square nextKnight(Side s);

  Square firstBishop(Side s);
  Square nextBishop(Side s);

  Square queen(Side s);
  Square king(Side s);


private:

  SideState white;
  SideState black;

};
