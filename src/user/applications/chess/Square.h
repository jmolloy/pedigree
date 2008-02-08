#ifndef SQUARE_H
#define SQUARE_H

class Square
{
public:
  Square(int col, int row) :
    col(col), row(row)
  {}

  ~Square() {}
  
  void rotate180()
  {
    col = 7-col;
    row = 7-row;
  }

  unsigned char col, row;
};

#endif
