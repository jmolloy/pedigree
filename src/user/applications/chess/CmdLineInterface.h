#ifndef CURSESINTERFACE_H
#define CURSESINTERFACE_H

#include "Interface.h"
#include "BoardState.h"

class CmdLineInterface : public Interface
{
public:
  CmdLineInterface();
  ~CmdLineInterface();

  virtual void start(char argc, char **argv);
private:

  BoardState importState();
};

#endif
