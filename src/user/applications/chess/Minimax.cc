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

#include "Minimax.h"
#include <stdio.h>

#ifdef DO_ALG
#undef DO_ALG
#endif
#define DO_ALG \
    for (Square sq2 = b.getAndClearFirstSetBit(); \
         !sq2.invalid(); \
         sq2 = b.getAndClearFirstSetBit()) \
    { \
      MoveList tmpList; \
      BoardState newState = state; \
      newState.move( Move(sq.col, sq.row, sq2.col, sq2.row), toPlay, Queen /* Default promotion piece */ ); \
      long tmpHeuristic = doSearch(side, curDepth+1, !isMaximising, maxDepth, newState, tmpList); \
      \
      if ( (isMaximising && tmpHeuristic > accumulator) || \
           (!isMaximising && tmpHeuristic < accumulator) ) \
      { \
        accumulator = tmpHeuristic; \
        moveList = tmpList; \
        moveList.push_front(Move(sq.col, sq.row, sq2.col, sq2.row)); \
      } \
    } 

Minimax::Minimax(volatile bool *keepGoing) :
  SearchAlgorithm(keepGoing)
{
}

Minimax::~Minimax()
{
}

long Minimax::search(Side side, int maxDepth, BoardState state, MoveList &moveList)
{
  nodesSearched = 0;
  return doSearch(side, 0, true, maxDepth, state, moveList);
}

long Minimax::doSearch(Side side, int curDepth, bool isMaximising, int maxDepth, BoardState state, MoveList &moveList)
{
  if (*m_KeepGoing == false)
    return 0;

  nodesSearched ++;

  // What side is to move?
  Side toPlay;
  if ( (side == White && isMaximising) || (side == Black && !isMaximising) )
    toPlay = White;
  else
    toPlay = Black;

  long h;
  if (state.load(maxDepth-curDepth, toPlay, moveList, h))
  {
    // Loading succeeded - we have a record of this BoardState evaluated to at least maxDepth-curDepth depth.
    return h;
  }

  // Check goal states.
  if (curDepth == maxDepth)
    return state.heuristic(side);
//   else if (state.inCheck(White) && // So that we don't do an inCheckmate check for every position!
//            state.inCheckmate(White))
//   {
//     return state.heuristic(side);
//   }
//   else if (state.inCheck(Black) && // So that we don't do an inCheckmate check for every position!
//            state.inCheckmate(Black))
//   {
//     return state.heuristic(side);
//   }
//   else if (state.inStalemate())
//   {
//     return state.heuristic();
//   }
  
  // For each piece type...
  Square sq;
  Bitboard b;
  long accumulator = (isMaximising) ? -10000000 : 10000000;

  for (sq = state.firstPawn(toPlay, &b);
       !sq.invalid();
       sq = state.nextPawn(toPlay, &b))
  {
    DO_ALG
  }
  for (sq = state.firstKnight(toPlay, &b);
       !sq.invalid();
       sq = state.nextKnight(toPlay, &b))
  {
    DO_ALG
  }
  for (sq = state.firstRook(toPlay, &b);
       !sq.invalid();
       sq = state.nextRook(toPlay, &b))
  {
    DO_ALG
  }
  for (sq = state.firstBishop(toPlay, &b);
       !sq.invalid();
       sq = state.nextBishop(toPlay, &b))
  {
    DO_ALG
  }
  for (sq = state.firstQueen(toPlay, &b);
       !sq.invalid();
       sq = state.nextQueen(toPlay, &b))
  {
    DO_ALG
  }
  sq = state.firstKing(toPlay, &b);
  DO_ALG

  // Return best move.
  state.save(maxDepth-curDepth, toPlay, accumulator, moveList); \
  return accumulator;

}
