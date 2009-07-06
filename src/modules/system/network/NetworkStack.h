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
#ifndef MACHINE_NETWORK_STACK_H
#define MACHINE_NETWORK_STACK_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <machine/Network.h>

/**
 * The Pedigree network stack
 * This function is the base for receiving packets, and provides functionality
 * for keeping track of network devices in the system.
 */
class NetworkStack
{
public:
  NetworkStack();
  virtual ~NetworkStack();
  
  /** For access to the stack without declaring an instance of it */
  static NetworkStack& instance()
  {
    return stack;
  }
  
  /** Called when a packet arrives */
  void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);

  /** Registers a given network device with the stack */
  void registerDevice(Network *pDevice);

  /** Returns the n'th registered network device */
  Network *getDevice(size_t n);

  /** Returns the number of devices registered with the stack */
  size_t getNumDevices();
  
  /** Unregisters a given network device from the stack */
  void deRegisterDevice(Network *pDevice);

private:

  static NetworkStack stack;

  /** Network devices registered with the stack. */
  Vector<Network*> m_Children;
};

#endif
