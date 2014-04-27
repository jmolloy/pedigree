/*
 * 
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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
