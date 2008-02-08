#ifndef MOVE_H
#define MOVE_H
/**
   Represents a piece moving across the board.
**/
class Move
{
public:
  Move(int sCol, int sRow, int eCol, int eRow) :
    sCol(sCol), sRow(sRow), eCol(eCol), eRow(eRow)
  {}

  ~Move() {}

  void rotate180()
  {
    sCol = 7-sCol;
    sRow = 7-sRow;
    eCol = 7-eCol;
    eRow = 7-eRow;
  }
  
  unsigned char sCol, sRow, eCol, eRow;
};

#endif
