#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <DebuggerCommand.h>

class DebuggerIO;

class DisassembleCommand : public DebuggerCommand
{
public:
  DisassembleCommand();
  ~DisassembleCommand();
  
  /**
   * Return an autocomplete string, given an input string.
   */
  void autocomplete(char *input, char *output, int len);

  /**
   * Execute the command with the given screen.
   */
  bool execute(char *input, char *output, int len, InterruptState &state, DebuggerIO *screen);
  
  /**
   * Returns the string representation of this command.
   */
  const char *getString();
};

#endif
