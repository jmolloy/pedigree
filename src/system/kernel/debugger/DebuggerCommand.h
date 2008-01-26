#ifndef DEBUGGER_COMMAND_H
#define DEBUGGER_COMMAND_H

class DebuggerCommand
{
public:
  DebuggerCommand() {};
  ~DebuggerCommand() {};
  
  /**
   * Return an autocomplete string, given an input string.
   */
  virtual void autocomplete(char *input, char *output) {};

  /**
   * Execute the command with the given screen.
   */
  virtual void execute(char *input, char *output) {};
  
  /**
   * Returns the string representation of this command.
   */
  virtual const char *getString() {};
};

#endif
