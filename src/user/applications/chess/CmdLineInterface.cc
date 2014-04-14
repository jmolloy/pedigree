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

#include "CmdLineInterface.h"
#include "Engine.h"
#include <stdio.h>

CmdLineInterface::CmdLineInterface()
{
}

CmdLineInterface::~CmdLineInterface()
{
}

void CmdLineInterface::start(char argc, char **argv)
{
    BoardState state = importState();
    Engine engine;
//     engine.state = state;

    engine.startSearch(White, 0, 40);

    while (!engine.searchComplete())
      ;

    engine.stopSearch();
    engine.printMoveList();
    
    engine.startSearch(White, 0, 4);

    while (!engine.searchComplete())
      ;

    engine.stopSearch();
    engine.printMoveList();

    engine.state.dump();
}

BoardState CmdLineInterface::importState()
{
  BoardState state;
  state.white.pawns.clear(); state.white.knights.clear(); state.white.rooks.clear();
  state.white.queen.clear(); state.white.king.clear(); state.white.bishops.clear();
  state.black.pawns.clear(); state.black.knights.clear(); state.black.rooks.clear();
  state.black.queen.clear(); state.black.king.clear(); state.black.bishops.clear();

  for (int i = 7; i >= 0; i--)
  {
    char c;
    for(int j = 0; j < 8; j++)
    {
      fread(&c, 1, 1, stdin);
      Square whiteSq(j, i);
      Square blackSq = whiteSq;
      blackSq.rotate180();
      switch (c)
      {
      case 'P': state.black.pawns.set(blackSq.row, blackSq.col); break;
      case 'R': state.black.rooks.set(blackSq.row, blackSq.col); break;
      case 'B': state.black.bishops.set(blackSq.row, blackSq.col); break;
      case 'H': state.black.knights.set(blackSq.row, blackSq.col); break;
      case 'Q': state.black.queen.set(blackSq.row, blackSq.col); break;
      case 'K': state.black.king.set(blackSq.row, blackSq.col); break;
      case 'p': state.white.pawns.set(whiteSq.row, whiteSq.col); break;
      case 'r': state.white.rooks.set(whiteSq.row, whiteSq.col); break;
      case 'b': state.white.bishops.set(whiteSq.row, whiteSq.col); break;
      case 'h': state.white.knights.set(whiteSq.row, whiteSq.col); break;
      case 'q': state.white.queen.set(whiteSq.row, whiteSq.col); break;
      case 'k': state.white.king.set(whiteSq.row, whiteSq.col); break;
      }
    }
    fread(&c, 1, 1, stdin);
  }
  return state;
}
