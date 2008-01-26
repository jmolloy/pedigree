#include <DisassembleCommand.h>
#include <utility.h>

DisassembleCommand::DisassembleCommand()
{
}

DisassembleCommand::~DisassembleCommand()
{
}

void DisassembleCommand::autocomplete(char *input, char *output)
{
  // TODO: add symbols.
  strcpy(output, "<address>");
}

void DisassembleCommand::execute(char *input, char *output)
{
}

const char *DisassembleCommand::getString()
{
  return "disassemble";
}
