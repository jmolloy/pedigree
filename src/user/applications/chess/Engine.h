#ifndef ENGINE_H
#define ENGINE_H

#include <pthread.h>
#include "Move.h"
#include "BoardState.h"
#include "SearchAlgorithm.h"

class Engine
{
public:
  /**
     Creates an engine, spawning a new thread and having that call run().
  **/
  Engine();
  
  /**
     Destroys an engine, killing the thread.
  **/
  ~Engine();

  /**
     Main runloop. Called solely by the spawned thread from Engine().
  **/
  void run();

  /**
     Enables or disables pondering.
  **/
  void ponder(bool b);
  
  /**
     Starts a new search, given the current board state.
  **/
  void startSearch(Side s, int minD, int maxD);

  /**
     Force an exit of the current search.
  **/
  void stopSearch();

  /**
     Is the search complete?
  **/
  bool searchComplete() volatile;

  /**
     Get the move the search recommends.
  **/
  Move getMove();

  /**
     Used to update the board position with either our or an opponent's move.
  **/
  void move(Move m);

  /**
     For debug purposes.
  **/
  void printMoveList();

  /**
    Trampoline.
  **/
  static void *threadTramp(void *ptr);

  /**
     The current state of the board.
  **/
  BoardState state;
private:
  /**
     The opening book that we can query.
  **/
  //OpeningBook book;

  /**
     The algorithm that does the brunt work for us.
  **/
  SearchAlgorithm *search;

  /**
     Should the worker thread be searching?
  **/
  volatile bool doSearch;
  
  /**
     The side to search for a move for.
  **/
  volatile Side side;
  
  /**
     The maximum and minimum depths to search to.
  **/
  volatile int minDepth, maxDepth;
  
  /**
     The move list that the engine found.
  **/
  MoveList moveList;
  long heuristic;

};

#endif
