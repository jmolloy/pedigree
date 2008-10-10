#ifndef SIDESTATE_H
#define SIDESTATE_H

#include "Bitboard.h"
#include "Move.h"
#include "Square.h"

enum Piece
{
  Pawn,
  Knight,
  Rook,
  Bishop,
  Queen,
  King
};

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
  void friendlyMove(Move m, Piece promotion=Pawn);
  void enemyMove(Move m, Bitboard enemyPawns);

  /**
   * Evaluates to true if the given Move is legal.
   */
  bool isLegal(Move m, Bitboard enemyPieces, Bitboard enemyPawnsEnPassant);
  
  /**
     Evaluates to true if the given Move is a castle.
  **/
  bool isCastle(Move m);
  
  /**
     Queries.
  **/
  bool isAttacking(Square sq, Bitboard enemyPieces); // Is (col,row) under attack by us?
  // TODO add en passant.
  
  long heuristic();
  
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
  bool hasCastled;

  /**
     Bitboards.
  **/
  Bitboard pawns;
  Bitboard rooks;
  Bitboard bishops;
  Bitboard knights;
  Bitboard queen;
  Bitboard king;
  Bitboard enPassant; // a bitboard giving squares which, if attacked, would result in a pawn being
                      // taken.

  /**
    All squares attacked by all pieces.
  **/
  Bitboard attack;

  /**
   * Bitboard for the next() function.
   */
  Bitboard nextBoard;
  int nextPiece;
  
};

#endif
