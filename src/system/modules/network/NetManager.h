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

#ifndef NETMANAGER_H
#define NETMANAGER_H

#include <vfs/Filesystem.h>
#include <utilities/RequestQueue.h>
#include <utilities/Vector.h>

#include <network/Endpoint.h>

#define NETMAN_PROTO_UDP    1
#define NETMAN_PROTO_TCP    2
#define NETMAN_PROTO_UDP6   10
#define NETMAN_PROTO_TCP6   11

/** Provides an interface to Endpoints for applications */
class NetManager : public Filesystem
{
public:
  NetManager()
  {
    m_Endpoints.clear();
  };

  virtual ~NetManager()
  {
  };

  static NetManager &instance()
  {
    return m_Instance;
  };

  //
  // ConsoleManager interface.
  //
  
  File newEndpoint(int protocol);
  
  bool isEndpoint(File f);
  
  Endpoint* getEndpoint(File f);
  
  void removeEndpoint(File f);
  
  File accept(File f);

  //
  // Filesystem interface.
  //

  virtual bool initialise(Disk *pDisk)
    {return false;}
  virtual File getRoot()
  {return File();}
  virtual String getVolumeLabel()
  {return String("netman");}
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer) { return 0; };
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer) { return 0; };
  virtual void truncate(File *pFile)
  {}
  virtual void fileAttributeChanged(File *pFile)
  {}
  virtual File getDirectoryChild(File *pFile, size_t n)
  {return File();}

protected:
  virtual bool createFile(File parent, String filename)
  {return false;}
  virtual bool createDirectory(File parent, String filename)
  {return false;}
  virtual bool createSymlink(File parent, String filename, String value)
  {return false;}
  virtual bool remove(File parent, File file)
  {return false;}

private:

  Vector<Endpoint*> m_Endpoints;
  static NetManager m_Instance;
};

#endif
