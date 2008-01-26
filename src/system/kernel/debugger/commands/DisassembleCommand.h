#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <DebuggerCommand.h>

class DisassembleCommand : public DebuggerCommand
{
public:
  DisassembleCommand();
  ~DisassembleCommand();
  
  /**
   * Return an autocomplete string, given an input string.
   */
  void autocomplete(char *input, char *output);

  /**
   * Execute the command with the given screen.
   */
  void execute(char *input, char *output);
  
  /**
   * Returns the string representation of this command.
   */
  const char *getString();
};

#endif
