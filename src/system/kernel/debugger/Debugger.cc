#include <Debugger.h>
#include <DebuggerIO.h>
#include <LocalIO.h>

Debugger::Debugger()
{
}

Debugger::~Debugger()
{
}

void Debugger::breakpoint(int type)
{

  DebuggerIO *pIo;
  LocalIO localIO;
  //SerialIO serialIO;
  if (type == MONITOR)
  {
    pIo = &localIO;
  }
  else
  {
    
  }

  pIo->enableCli();

  for(;;);

}

