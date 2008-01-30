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

#ifndef DEBUGGER_H
#define DEBUGGER_H

#define MONITOR 1
#define SERIAL  2

extern class Debugger g_Debugger;

/**
 * Implements the main kernel debugger. This class interfaces with the machine
 * abstraction to provide trap and breakpoint services. It exposes a set of commands,
 * which are accessed by the user through a command-line interface defined by a concretion
 * of the abstract class DebuggerIO.
 * The kernel can also write entries to the (debug) system log by calling operator<< directly.
 */
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
   * \param type One of MONITOR or SERIAL. It defines the type of interface that will be started for
   *             user communication.
   */
  void breakpoint(int type);

};

#endif

