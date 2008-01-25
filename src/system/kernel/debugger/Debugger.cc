/*
 * Copyright (c) 2008 James Molloy
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

#include <Debugger.h>
#include <DebuggerIO.h>
#include <LocalIO.h>
#include <utility.h>

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
  
  pIo->setCliUpperLimit(1);
  pIo->setCliLowerLimit(1);
  pIo->enableCli();
  char str[64];
/*  for (int i = 0; i < 100; i++)
  {
    sprintf(str, "James is cool %d\n", i);
    pIo->writeCli(str, DebuggerIO::White, DebuggerIO::Black);
  }*/
  while(1)
    pIo->readCli(0, 0);

//   pIo->drawString("Yes, i am cheating!", 2, 70, DebuggerIO::Yellow, DebuggerIO::Blue);
  for(;;);

}

