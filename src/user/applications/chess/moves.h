#ifndef MOVES_H
#define MOVES_H

#include "Bitboard.h"
#include "Square.h"

void initLookupTable();
Bitboard rookMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard bishopMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard queenMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard knightMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos);
Bitboard kingMoves(Bitboard allPieces, Bitboard enemyPieces, Square pos, bool leftRookMoved,
                   bool rightRookMoved, bool kingMoved);
Bitboard pawnMoves(Bitboard allPieces, Bitboard enemyPieces, Bitboard enemyPawnsEnPassant, Square pos);
Bitboard pawnAttackOnly(Bitboard allPieces, Bitboard enemyPieces, Bitboard enemyPawnsEnPassant, Square pos);

#endif
