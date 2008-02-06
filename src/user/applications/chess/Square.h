#ifndef SQUARE_H
#define SQUARE_H

class Square
{
public:
  Square(int col, int row) :
    col(col), row(row)
  {}

  ~Square() {}

  unsigned char col, row;
};

#endif
