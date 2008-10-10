#include <stdio.h>
#include "Bitboard.h"
#include "Square.h"
#include "moves.h"
#include "Engine.h"
#include "CmdLineInterface.h"
#include "XboardInterface.h"
#include "StateStore.h"
#include "hashes.h"
#include "BoardState.h"

StateStore stateStore(20); // 1million entries

int main (char argc, char **argv)
{
  initLookupTable();
  
  Interface *pInterface = 0;

  XboardInterface interface;
  pInterface = &interface;

  pInterface->start(argc, argv);
}
