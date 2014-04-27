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

#ifndef SEARCHALGORITHM_H
#define SEARCHALGORITHM_H

#include <list>
#include "Move.h"

typedef std::list<Move> MoveList;

#include "BoardState.h"

/**
   Abstract class to allow multiple algorithms to be used.
**/
class SearchAlgorithm
{
public:
  SearchAlgorithm(volatile bool *keepGoing);
  virtual ~SearchAlgorithm() {}
  
  virtual long search(Side side, int maxDepth, class BoardState state, MoveList &moveList) = 0;
  
  unsigned long nodesSearched;
  
protected:
  volatile bool *m_KeepGoing;
};

#endif
