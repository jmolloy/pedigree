/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#ifdef THREADS
#include <process/Mutex.h>
#endif
#include <Log.h>
#ifdef OPENFIRMWARE
#include <machine/openfirmware/OpenFirmware.h>
#endif

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
    Generic,    ///< The device type is not covered by any other value.
    Root,       ///< The device is the root of the device tree.
    Disk,       ///< A disk device - a block device in UNIX terms.
    Bus,        ///< A node that performs address space translation and I/O multiplexing.
    Display,    ///< A display device. This can either be a dumb framebuffer or something more complex.
    Network,    ///< A communication device.
    Sound,      ///< A device that can play sounds.
    Console,    ///< A keyboard-like human interface device.
    Mouse,      ///< A mouse-like human interface device (includes trackpads and styluses)
    Controller, ///< A device which exposes other devices but requires a driver, and does no address space translation.
    UsbController, ///< Same as Controller, but exposes USB devices.
    UsbGeneric, ///< Same as Generic, but for USB devices.
    UsbContainer,///< Wraps a UsbDevice (which does not inherit Device) in a Device so it can be in the tree.
  };
  /** Each device may have one or more disjoint regions of address space. This can be in I/O space
      or memory space. */
  class Address
  {
  public:
    /** Constructor, takes arguments and assigns them blindly to member variables, then
     *  creates the IoPort or MemoryMappedIO instance.  */
    Address(String n, uintptr_t a, size_t s, bool io, size_t pad=1);
    ~Address();
    /** Actually maps this address into memory if not already done. */
    void map(size_t forcedSize = 0, bool bUser = false, bool bWriteCombine = false, bool bWriteThrough = false);
    /** A textual identifier for this address range. */
    String m_Name;
    /** The base of the address range, as the processor sees it (all parental
        address space transformations applied) */
    uintptr_t m_Address;
    /** The length of the address range. */
    size_t m_Size;
    /** True if the address range is in I/O space, false if in memory space */
    bool m_IsIoSpace;
    /** Either IoPort or MemoryMappedIO depending on the address space type. */
    IoBase *m_Io;
    /** Some devices' registers aren't contiguous in memory, but padded to boundaries. */
    size_t m_Padding;
  private:
    Address(const Address &);
    Address &operator = (const Address &);
    
    bool m_bMapped;
  };

  Device();
  /// \warning This renders 'p' unusable - it deletes all its IoPorts and MemoryMappedIo's.
  ///          This is because the new Device must have access to the IoBases of the old
  ///          device, and multiple instances of the same IoBase cannot be shared!
  Device(Device *p);
  virtual ~Device();

  /** Retrieves the root device */
  static Device &root() DEPRECATED
  {
    return m_Root;
  }

  /**
   * Traverses the full device tree, calling the given callback for each item.
   *
   * This will take a lock if threading is enabled, such that any operations
   * taking place on the device tree will block until iteration completes.
   *
   * To facilitate environments that may need to replace objects, the callback
   * returns a Device pointer. If this pointer is null, the referenced Device
   * is removed from the tree. If this pointer is different to the original,
   * the original is replaced by the new pointer in the tree. Otherwise, if
   * the pointer does not differ, no action is taken.
   *
   * This characteristic allows for a full traversal to be performed, editing
   * the tree along the way, safely and without conflicting with other attempts
   * to edit the tree.
   *
   * You, in almost every case, want this function if the Device you're editing
   * is already linked into the Device tree.
   *
   * \todo add filters to avoid the need to filter in callbacks
   */
  static void foreach(Device *(*callback)(Device *), Device *root = 0);

  /** Adds the given object to the root of the device tree, atomically. */
  static void addToRoot(Device *device);

  /** Returns the device's parent */
  inline Device *getParent() const
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

  /** PCI-specific location identifiers **/
  void setPciPosition(uint32_t bus, uint32_t device, uint32_t func)
  {
    m_PciBusPos = bus;
    m_PciDevicePos = device;
    m_PciFunctionNum = func;
  }
  /** PCI-specific identifiers - class code, subclass code, vendor and device ID **/
  void setPciIdentifiers(uint8_t classCode, uint8_t subclassCode, uint16_t vendorId, uint16_t deviceId, uint8_t progIf)
  {
    m_ClassCode = classCode;
    m_SubclassCode = subclassCode;
    m_VendorId = vendorId;
    m_DeviceId = deviceId;
    m_ProgInterface = progIf;
  }
  /** Returns the PCI class code. */
  inline uint8_t getPciClassCode()
  {
    return m_ClassCode;
  }
  /** Returns the PCI subclass code. */
  inline uint8_t getPciSubclassCode()
  {
    return m_SubclassCode;
  }
  /** Returns the PCI vendor ID. */
  inline uint16_t getPciVendorId()
  {
    return m_VendorId;
  }
  /** Returns the PCI device ID. */
  inline uint16_t getPciDeviceId()
  {
    return m_DeviceId;
  }
  /** Returns the PCI programming interface. */
  inline uint8_t getPciProgInterface()
  {
    return m_ProgInterface;
  }
  /** Returns the PCI bus position for this device. */
  inline uint32_t getPciBusPosition()
  {
    return m_PciBusPos;
  }
  /** Returns the PCI device position on the bus for this device. */
  inline uint32_t getPciDevicePosition()
  {
    return m_PciDevicePos;
  }
  /** Returns the PCI Function number for this device. */
  inline uint32_t getPciFunctionNumber()
  {
    return m_PciFunctionNum;
  }

  /** Dumps a textual representation of the device into the given string. */
  virtual void dump(String &str)
  {
    str = "Abstract Device";
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

  /** Sets pDevice as a child of this device. pDevice's parent is NOT updated. */
  void addChild(Device *pDevice);

  /** Returns the n'th child of this device, or 0 if the device has less than n children. */
  Device *getChild(size_t n);

  /** Returns the number of children this device has. */
  size_t getNumChildren();

  /** Removes the n'th child from this device. The result is undefined if the device does not have n children. */
  void removeChild(size_t n);

  /** Attempts to find the device d, if found, removes it. */
  void removeChild(Device *d);

  /** Attempts to find the device src in this device's children. If found, it replaces 'src' with 'dest' in this device's child list, so that 'src' is no longer a child of this device, and 'dest' is, in the same position that 'src' was. */
  void replaceChild(Device *src, Device *dest);

  /** Search functions */
  void searchByVendorId(uint16_t vendorId, void (*callback)(Device*));

  void searchByVendorIdAndDeviceId(uint16_t vendorId, uint16_t deviceId, void (*callback)(Device*));

  void searchByClass(uint16_t classCode, void (*callback)(Device*));

  void searchByClassAndSubclass(uint16_t classCode, uint16_t subclassCode, void (*callback)(Device*));

  void searchByClassSubclassAndProgInterface(uint16_t classCode, uint16_t subclassCode, uint8_t progInterface, void (*callback)(Device*));

#ifdef OPENFIRMWARE
  /** Gets the device's OpenFirmware handle. */
  OFHandle getOFHandle()
  {
    return m_OfHandle;
  }
  /** Sets the device's OpenFirmware handle. */
  void setOFHandle(OFHandle h)
  {
    m_OfHandle = h;
  }
#endif
private:
  /** Actual do-er for foreach (does not take lock). */
  static void foreachInternal(Device *(callback)(Device *), Device *root);

  /** Copy constructor.
      \note NOT implemented. */
  Device(const Device&);
  /** Assignment operator.
      \note NOT implemented. */
  void operator=(const Device &);

  /** Destroys all IoBases in this class. Called from the constructor Device(Device*). */
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
#ifdef OPENFIRMWARE
  /** OpenFirmware device handle. */
  OFHandle m_OfHandle;
#endif
  /** PCI Device class */
  uint8_t m_ClassCode;
  /** PCI subclass */
  uint8_t m_SubclassCode;
  /** PCI Vendor ID */
  uint16_t m_VendorId;
  /** PCI Device ID */
  uint16_t m_DeviceId;
  /** PCI Programming Interface */
  uint8_t m_ProgInterface;

  /** PCI Bus number */
  uint32_t m_PciBusPos;
  /** PCI Device number */
  uint32_t m_PciDevicePos;
  /** PCI Function number */
  uint32_t m_PciFunctionNum;
#ifdef THREADS
  /** Lock to manage access to the device tree. */
  static Mutex m_TreeLock;
#endif
};

#endif
