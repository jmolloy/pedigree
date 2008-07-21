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
#include <processor/IoBase.h>

/**
 * Represents a node in the device tree. This could either be a bus (non-leaf node) or a
 * device (leaf node).
 */
class Device
{
public:
  /** Every device has a type. This can be used to downcast to a more specific class
   * during runtime without RTTI. */
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
  /** Each device may have one or more disjoint regions of address space. This can be in I/O space
      or memory space. */
  class Address
  {
  public:
    /** Constructor, takes arguments and assigns them blindly to member variables, then
     *  creates the IoPort or MemoryMappedIO instance.  */
    Address(String n, uintptr_t a, size_t s, bool io);
    ~Address();
    /** A textual identifier for this address range. */
    String m_Name;
    /** The base of the address range. */
    uintptr_t m_Address;
    /** The length of the address range. */
    size_t m_Size;
    /** True if the address range is in I/O space, false if in memory space */
    bool m_IsIoSpace;
    /** Either IoPort or MemoryMappedIO depending on the address space type. */
    IoBase *m_Io;
  };
  
  Device();
  /// \warning This renders 'p' unusable - it deletes all its IoPorts and MemoryMappedIo's.
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
  
  /** Returns the (specific) type of the device, in string form. */
  virtual String getSpecificType()
  {
    return m_SpecificType;
  }
  /** Sets the specific type of the device, in string form. */
  virtual void setSpecificType(String str)
  {
    m_SpecificType = str;
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
  
  /** Returns the interrupt number of the device. */
  virtual uintptr_t getInterruptNumber()
  {
    return m_InterruptNumber;
  }
  
  /** Sets the interrupt number of the device. */
  virtual void setInterruptNumber(uintptr_t n)
  {
    m_InterruptNumber = n;
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
  
  void removeIoMappings();

protected:
  /** The address of this device, in its parent's address space. */
  Vector<Address*> m_Addresses;
  /** The children of this device. */
  Vector<Device*> m_Children;
  /** This device's parent. */
  Device *m_pParent;
  /** The root node. */
  static Device m_Root;
  /** The interrupt number */
  uintptr_t m_InterruptNumber;
  /** The specific device type */
  String m_SpecificType;
};

#endif
