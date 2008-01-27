#include <DisassembleCommand.h>
#include <DebuggerIO.h>
#include <utility.h>
#include <udis86.h>

DisassembleCommand::DisassembleCommand()
{
}

DisassembleCommand::~DisassembleCommand()
{
}

void DisassembleCommand::autocomplete(char *input, char *output, int len)
{
  // TODO: add symbols.
  strcpy(output, "<address>");
}

bool DisassembleCommand::execute(char *input, char *output, int len, DebuggerIO *screen)
{
  // This command can take either an address or a symbol name.
  // Is it an address?
  unsigned long address = strtoul(input, 0, 0);

  if (address == 0)
  {
    // No, try a symbol name.
    // TODO.
    sprintf(output, "Not a valid address or symbol name: `%s'.\n", input);
    return true;
  }

  
  // Dissassemble around address.
  int nInstructions = 10;
  
  ud_t ud_obj;
  ud_init(&ud_obj);
#ifdef X86
  ud_set_mode(&ud_obj, 32);
#endif
#ifdef X86_64
  ud_set_mode(&ud_obj, 64);
#endif
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);
  ud_set_pc(&ud_obj, (unsigned long long)address);
  ud_set_input_buffer(&ud_obj, (unsigned char *)address, 256);
  
  output[0] = '\0';
  for(int i = 0; i < nInstructions; i++)
  {
    ud_disassemble(&ud_obj);
    
    char addr[32];
#ifdef X86
    sprintf(addr, "%08x", ud_insn_off(&ud_obj)); 
#endif
#ifdef X86_64
    sprintf(addr, "%016x", ud_insn_off(&ud_obj));
#endif
    strcat(output, addr);
    strcat(output, ": ");
    strcat(output, ud_insn_asm(&ud_obj));
    strcat(output, "\n");
  }
  
  return true;
}

const char *DisassembleCommand::getString()
{
  return "disassemble";
}
