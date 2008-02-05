
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

  unsigned char sCol, sRow, eCol, eRow;
}
