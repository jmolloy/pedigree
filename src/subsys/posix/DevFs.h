#ifndef DEVFS_H
#define DEVFS_H

#include <vfs/Filesystem.h>
#include <vfs/File.h>

class RandomFile : public File
{
    public:
        RandomFile(String str, Filesystem *pParent) :
            File(str, 0, 0, 0, 0, pParent, 0, 0)
        {}
        ~RandomFile()
        {}

        uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
        uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
};

/** This class provides /dev/urandom */
class RandomFs : public Filesystem
{
    public:
        RandomFs() :
            m_pRandom(0)
        {};

        virtual ~RandomFs()
        {
            if(m_pRandom)
                delete m_pRandom;
        }

        static RandomFs &instance()
        {
            return m_Instance;
        }

        File* getFile()
        {
            if (m_pRandom) return m_pRandom;
            m_pRandom = new RandomFile(String("/dev/urandom"), this);
            return m_pRandom;
        }

        virtual bool initialise(Disk *pDisk)
        {return false;}
        virtual File* getRoot()
        {return 0;}
        virtual String getVolumeLabel()
        {return String("urandom");}
        virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true)
        {
            // http://xkcd.com/221/
            void *pBuffer = reinterpret_cast<void *>(buffer);
            memset(pBuffer, 4, size);
            return size;
        }
        virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true)
        {
            return size;
        }

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

        RandomFs(const RandomFs &);
        RandomFs &operator = (const RandomFs &);

        File *m_pRandom;
        static RandomFs m_Instance;
};

class NullFile : public File
{
public:
    NullFile(String str, Filesystem *pParent) :
        File(str, 0, 0, 0, 0, pParent, 0, 0)
    {}
    ~NullFile()
    {}

    uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
    uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
};

/** This class provides /dev/null */
class NullFs : public Filesystem
{
public:
  NullFs() :
    m_pNull(0)
  {};

  virtual ~NullFs()
  {
    if(m_pNull)
        delete m_pNull;
  };

  static NullFs &instance()
  {
    return m_Instance;
  }

  File* getFile()
  {
    if (m_pNull) return m_pNull;
    m_pNull = new NullFile(String("/dev/null"), this);
    return m_pNull;
  }

  virtual bool initialise(Disk *pDisk)
  {return false;}
  virtual File* getRoot()
  {return 0;}
  virtual String getVolumeLabel()
  {return String("nullfs");}
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true)
  {
    memset(reinterpret_cast<void*>(buffer), 0, size);
    return size;
  }
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true)
  {
    return size;
  }

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
