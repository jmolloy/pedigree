/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <machine/initialise.h>


#include <Log.h>
/*
/**
 * Possible machine list for MIPS.
 
#ifdef X86_COMMON
  #include <machine/pc/Pc.h>
  Pc g_Pc;
  Machine *g_pPossibleMachines[] = {&g_Pc};
  size_t g_nPossibleMachines = 1;
#endif

/**
   * Possible machine list for MIPS.
 
#ifdef MIPS_COMMON
  #include <machine/malta/Malta.h>
  #include <machine/malta/Bonito64.h>
  #include <machine/au1500/Db1500.h>
  Malta g_Malta;
  Bonito64 g_Bonito64;
  Db1500 g_Db1500;
  Machine *g_pPossibleMachines[] = {&g_Malta, &g_Bonito64, &g_Db1500};
  size_t g_nPossibleMachines = 3;
#endif

bool machineIsInitialised = false;
Machine *g_pMachine;

void initialiseMachine()
{
  bool bFound = false;
  for (int i = 0; i < g_nPossibleMachines; i++)
    if (g_pPossibleMachines[i]->probe())
    {
      g_pMachine = g_pPossibleMachines[i];
      bFound = true;
    }
  if (!bFound)
  {
    FATAL("No possible machine found!");
    return;
  }
  
  g_pMachine->initialise();

  machineIsInitialised = true;
}

bool isMachineInitialised()
{
  return machineIsInitialised;
}*/
