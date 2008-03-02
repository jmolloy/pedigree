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
  void autocomplete(const HugeStaticString &input, HugeStaticString &output);

  /**
   * Execute the command with the given screen.
   */
  bool execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *screen);
  
  /**
   * Returns the string representation of this command.
   */
  const NormalStaticString getString();
};

#endif
