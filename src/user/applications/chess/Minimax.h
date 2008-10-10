#ifndef MINIMAX_H
#define MINIMAX_H

#include "BoardState.h"
#include "SearchAlgorithm.h"

class Minimax : public SearchAlgorithm
{
public:
  Minimax(volatile bool *keepGoing);
  ~Minimax();
  
  virtual long search(Side side, int maxDepth, BoardState state, MoveList &moveList);
  
private:
  long doSearch(Side side, int curDepth, bool isMaximising, int maxDepth, BoardState state, MoveList &moveList);
};

#endif
