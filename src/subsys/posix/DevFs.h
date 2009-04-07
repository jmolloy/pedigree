#ifndef DEVFS_H
#define DEVFS_H

#include <vfs/Filesystem.h>

/** This class provides /dev/null */
class NullFs : public Filesystem
{
public:
  NullFs() :
    m_pNull(0)
  {};

  virtual ~NullFs()
  {};

  static NullFs &instance()
  {
    return m_Instance;
  }

  File* getFile()
  {
    if (m_pNull) return m_pNull;
    m_pNull = new File(String("/dev/null"), 0, 0, 0, 0, this, 0, 0);
    return m_pNull;
  }

  virtual bool initialise(Disk *pDisk)
  {return false;}
  virtual File* getRoot()
  {return 0;}
  virtual String getVolumeLabel()
  {return String("consolemanager");}
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
  {
    memset(reinterpret_cast<void*>(buffer), 0, size);
    return size;
  }
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
  {
    return size;
  }
  virtual void truncate(File *pFile)
  {}
  virtual void fileAttributeChanged(File *pFile)
  {}
  virtual void cacheDirectoryContents(File *pFile)
  {}

protected:
  virtual bool createFile(File* parent, String filename, uint32_t mask)
  {return false;}
  virtual bool createDirectory(File* parent, String filename)
  {return false;}
  virtual bool createSymlink(File* parent, String filename, String value)
  {return false;}
  virtual bool remove(File* parent, File* file)
  {return false;}

private:

  NullFs(const NullFs &);
  NullFs &operator = (const NullFs &);

  File *m_pNull;
  static NullFs m_Instance;
};

#endif
