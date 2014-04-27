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

#include "Engine.h"
#include <stdlib.h>
#include <stdio.h>
#include "Minimax.h"
#include "Negascout.h"
#include "Alphabeta.h"

extern long g_nCacheHits, g_nCacheMisses, g_nCacheEntries, g_nCachePartials;
extern long g_nHashes, g_nIsLegal, g_nInCheck;
extern long g_nIsAttacking, g_nHeuristic;

Engine::Engine()
{
  // Initialise the algorithm.
//   search = new Minimax(&doSearch);
  search = new Negascout(&doSearch);
//   search = new Alphabeta(&doSearch);
  doSearch = false;
  minDepth = 0;
  maxDepth = 0;

  pthread_t thread;
  pthread_create(&thread, NULL, &threadTramp, (void*)this);
}

Engine::~Engine()
{
}

void Engine::run()
{
  while (true)
  {
    while (!doSearch)
      ;

    // Iterative deepening solution.
    for (int ply = 1; ply <= maxDepth; ply++)
    {
      if (doSearch)
      {
        MoveList tmpList;
        heuristic = search->search(side, ply, state, tmpList);
        moveList = tmpList;
        printf("[%d] ", ply);
        printMoveList();
        g_nCacheHits = 0;
        g_nCacheMisses = 0;
        g_nCachePartials = 0;
        g_nHashes = 0;
        g_nInCheck = 0;
        g_nIsLegal = 0;
        g_nIsAttacking = 0;
        g_nHeuristic = 0;
      }
    }

    doSearch = false;
  }
}

void Engine::ponder(bool b)
{
}

void Engine::startSearch(Side s, int minD, int maxD)
{
  side = s;
  minDepth = minD;
  maxDepth = maxD;
  doSearch = true;
}

void Engine::stopSearch()
{
  doSearch = false;
}

bool Engine::searchComplete() volatile
{
  return !doSearch; // If we're still searching, it's not complete.
}

Move Engine::getMove()
{
  return *moveList.begin();
}

void Engine::move(Move m)
{
}

void Engine::printMoveList()
{
  for (MoveList::iterator it = moveList.begin();
       it != moveList.end();
       it ++)
  {
    Move m = *it;
    m.print();
    printf(" ");
  }
  printf("(Heuristic %d - Searched %d nodes with %d cache hits, %d partials and %d misses (%d entries))\n", heuristic, search->nodesSearched, g_nCacheHits, g_nCachePartials, g_nCacheMisses, g_nCacheEntries);
  printf("{%d hashes %d islegal %d incheck %d isattacking %d heuristic}\n", g_nHashes, g_nIsLegal, g_nInCheck, g_nIsAttacking, g_nHeuristic);
}

void *Engine::threadTramp(void *ptr)
{
  Engine *engine = (Engine*)ptr;
  engine->run();
  return 0;
}
