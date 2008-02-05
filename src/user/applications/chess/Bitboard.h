#include <stdint.h>

class Bitboard
{
public:
  /**
     Constructs a bitboard, intialising to all zeroes.
  **/
  Bitboard();

  /**
     Constructs a bitboard, taking an array of bytes, each representing one
     square.
  **/
  Bitboard(unsigned char squares[64]);

  ~Bitboard() {}

  /**
     Retrieves a rank in byte form.
  **/
  unsigned char getRank(int n)
  {
    return member.c[n];
  }

  /**
     Sets a rank in byte form.
  **/
  void setRank(int n, unsigned char c)
  {
    member.c[n] = c;
  }

  /**
     Tests the bit at rank, file
  **/
  bool test(int rank, int file)
  {
    return (bool) (member.c[rank] & (0x80 >> file));
  }

  /**
     Clears the bit at rank, file
  **/
  bool clear(int rank, int file)
  {
    member.c[rank] &= ~(0x80 >> file);
  }

  /**
     Sets the bit at rank, file
  **/
  bool set(int rank, int file)
  {
    member.c[rank] |= 0x80 >> file;
  }

  /**
     Flips the board 180 degrees.
  **/
  void flip180();

  unsigned char getDiagonalRank45(int rank, int file, int *newFile);
  unsigned char getDiagonalRank315(int rank, int file, int *newFile);
  void setDiagonalRank45(int rank, int file, unsigned char newRank);
  void setDiagonalRank315(int rank, int file, unsigned char newRank);
  
  /**
    Debugging function - prints out a bitboard.
  **/
  void print();

  /**
    Overloaded bitwise functions.
  **/
  Bitboard operator|(Bitboard &b2);

private:
  union
  {
    uint64_t i;
    unsigned char c[8];
  } member;
};
