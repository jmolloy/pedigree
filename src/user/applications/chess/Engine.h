
#include <pthread.h>
#include <Move.h>
#include <BoardState.h>

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
  void startSearch();

  /**
     Force an exit of the current search.
  **/
  void stopSearch();

  /**
     Is the search complete?
  **/
  bool searchComplete();

  /**
     Get the move the search reccommends.
  **/
  Move getMove();

  /**
     Used to update the board position with either our or an opponent's move.
  **/
  void move(Move m);

private:
  /**
     The current state of the board.
  **/
  BoardState state;

  /**
     The StateStore that we can query and save states to.
  **/
  //StateStore store;

  /**
     The opening book that we can query.
  **/
  //OpeningBook book;

  /**
     The algorithm that does the brunt work for us.
  **/
  class SearchAlgorithm *search;

}
