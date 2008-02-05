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

  /**
     Rotates the bitboard by 45 degrees
  **/
  void rotate45();

  /**
     Rotates the bitboard by 315 degrees
  **/
  void rotate315();

private:
  union
  {
    uint64_t i;
    unsigned char c[8];
  } member;
};
