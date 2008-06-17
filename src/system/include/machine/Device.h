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
#ifndef MACHINE_DEVICE_H
#define MACHINE_DEVICE_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>

/**
 * Represents a node in the device tree. This could either be a bus (non-leaf node) or a
 * device (leaf node).
 */
class Device
{
public:
  enum Type
  {
    Generic,
    Root,
    Disk,
    Bus,
    Display,
    Network,
    Sound,
    Keyboard,
    Controller
  };
  class Address
  {
  public:
    Address(String n, uintptr_t a, size_t s) :
      m_Name(n), m_Address(a), m_Size(s)
    {}
    ~Address() {}
    String m_Name;
    uintptr_t m_Address;
    size_t m_Size;
  };
  
  Device();
  Device(Device *p);
  virtual ~Device();

  /** Retrieves the root device */
  static Device &root()
  {
    return m_Root;
  }
  
  /** Returns the device's parent */
  inline Device *getParent()
  {
    return m_pParent;
  }
  /** Sets the device's parent. */
  inline void setParent(Device *p)
  {
    m_pParent = p;
  }

  /** Stores the device's name in str. */
  virtual void getName(String &str);
  
  /** Returns the (abstract) type of the device. */
  virtual Type getType()
  {
    return Root;
  }
  
  /** Dumps a textual representation of the device into the given string. */
  virtual void dump(String &str)
  {
  }

  /** Returns the addresses of the device, in its parent's address space */
  virtual Vector<Address*> &addresses()
  {
    return m_Addresses;
  }

  void addChild(Device *pDevice);
  Device *getChild(size_t n);
  size_t getNumChildren();
  void removeChild(size_t n);
  void removeChild(Device *d);
  void replaceChild(Device *src, Device *dest);

private:
  Device(const Device&);
  void operator=(const Device &);

protected:
  /** The address of this device, in its parent's address space. */
  Vector<Address*> m_Addresses;
  /** The children of this device. */
  Vector<Device*> m_Children;
  /** This device's parent. */
  Device *m_pParent;
  /** The root node. */
  static Device m_Root;
};

#endif
