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
