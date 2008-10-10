#ifndef SQUARE_H
#define SQUARE_H

class Square
{
public:
  Square() :
    col(-1), row(-1)
  {}
  
  Square(int col, int row) :
    col(col), row(row)
  {}

  ~Square() {}
  
  void rotate180()
  {
    col = 7-col;
    row = 7-row;
  }

  bool invalid()
  {
    return (col == -1 && row == -1);
  }

  char col, row;
};

#endif
