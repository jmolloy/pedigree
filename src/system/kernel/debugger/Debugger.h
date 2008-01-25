#ifndef DEBUGGER_H
#define DEBUGGER_H

#define MONITOR 1
#define SERIAL  2

class Debugger
{
public:
  /**
   * Default constructor/destructor - does nothing.
   */
  Debugger();
  ~Debugger();

  /**
   * Causes the debugger to take control.
   */
  void breakpoint(int type);

};

#endif

