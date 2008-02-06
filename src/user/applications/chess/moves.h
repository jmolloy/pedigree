#ifndef MOVES_H
#define MOVES_H

#include "Bitboard.h"
#include "Square.h"

Bitboard rookMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard bishopMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard queenMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard knightMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard kingMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos, bool leftRookMoved,
                   bool rightRookMoved, bool kingMoved);

#endif
