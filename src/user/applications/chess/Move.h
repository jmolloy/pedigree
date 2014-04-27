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

#ifndef MOVE_H
#define MOVE_H

#include <stdio.h>

/**
   Represents a piece moving across the board.
**/
class Move
{
public:
  Move(int sCol, int sRow, int eCol, int eRow) :
    sCol(sCol), sRow(sRow), eCol(eCol), eRow(eRow)
  {}
  
  Move() :
    sCol(-1), sRow(-1), eCol(-1), eRow(-1)
  {}

  ~Move() {}

  void rotate180()
  {
    sCol = 7-sCol;
    sRow = 7-sRow;
    eCol = 7-eCol;
    eRow = 7-eRow;
  }
  
  void print()
  {
    switch (sCol)
    {
    case 0: printf ("A"); break;
    case 1: printf ("B"); break;
    case 2: printf ("C"); break;
    case 3: printf ("D"); break;
    case 4: printf ("E"); break;
    case 5: printf ("F"); break;
    case 6: printf ("G"); break;
    case 7: printf ("H"); break;
    }
    printf("%d-", sRow);
    
    switch (eCol)
    {
    case 0: printf ("A"); break;
    case 1: printf ("B"); break;
    case 2: printf ("C"); break;
    case 3: printf ("D"); break;
    case 4: printf ("E"); break;
    case 5: printf ("F"); break;
    case 6: printf ("G"); break;
    case 7: printf ("H"); break;
    }
    printf("%d", eRow);
  }
  
  bool invalid()
  {
    return (sCol == (char)-1);
  }
  
  char sCol, sRow, eCol, eRow;
};

#endif
