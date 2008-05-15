/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include <processor/Processor.h>
#include <processor/state.h>
#include <processor/InterruptManager.h>
#include <LocalIO.h>

/** @addtogroup kerneldebugger
 * @{ */

#define ASSERT_FAILED_SENTINEL 0xa55e4710 // A cack-handed way of writing "assertio(n)".

#ifdef DEBUGGER
/**
 * Implements the main kernel debugger. This class interfaces with the machine
 * abstraction to provide trap and breakpoint services. It exposes a set of commands,
 * which are accessed by the user through a command-line interface defined by a concretion
 * of the abstract class DebuggerIO.
 * The kernel can also write entries to the (debug) system log by calling operator<< directly.
 */
class Debugger : public InterruptHandler
{
public:
  /**
   * Get the instance of the Debugger
   */
  inline static Debugger &instance()
  {
    return m_Instance;
  }
  
  /**
   * Initialise the debugger - register interrupt handlers etc.
   */
  void initialise();
  
  /**
   * Causes the debugger to take control.
   */
  void start(InterruptState &state, LargeStaticString &description);
   
  /** Called when the handler is registered with the interrupt manager and the interrupt occurred
   * \param interruptNumber the interrupt number
   * \param state reference to the state before the interrupt
   */
  virtual void interrupt(size_t interruptNumber, InterruptState &state);

  InterruptState *m_pTempState;

private:
  /**
   * Default constructor - does nothing.
   */
  Debugger();
  Debugger(const Debugger &);
  Debugger &operator = (const Debugger &);
  ~Debugger();

  /**
   * The current DebuggerIO type.
   */
  int m_nIoType;

  /**
   * The Debugger instance (singleton class)
   */
  static Debugger m_Instance;
};

/** @} */

#endif

#endif

