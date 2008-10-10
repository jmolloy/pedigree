#ifndef SEARCHALGORITHM_H
#define SEARCHALGORITHM_H

#include <list>
#include "Move.h"

typedef std::list<Move> MoveList;

#include "BoardState.h"

/**
   Abstract class to allow multiple algorithms to be used.
**/
class SearchAlgorithm
{
public:
  SearchAlgorithm(volatile bool *keepGoing);
  virtual ~SearchAlgorithm() {}
  
  virtual long search(Side side, int maxDepth, class BoardState state, MoveList &moveList) = 0;
  
  unsigned long nodesSearched;
  
protected:
  volatile bool *m_KeepGoing;
};

#endif
