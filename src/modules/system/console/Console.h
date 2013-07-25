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
#ifndef CONSOLE_H
#define CONSOLE_H

#include <vfs/VFS.h>
#include <vfs/File.h>
#include <vfs/Filesystem.h>
#include <utilities/RequestQueue.h>
#include <utilities/Vector.h>
#include <utilities/RingBuffer.h>
#include <Spinlock.h>

#define CONSOLE_READ    1
#define CONSOLE_WRITE   2
#define CONSOLE_GETROWS 3
#define CONSOLE_GETCOLS 4
#define CONSOLE_DATA_AVAILABLE 5
#define CONSOLE_REFRESH 10
#define CONSOLE_FLUSH   11

#define LINEBUFFER_MAXIMUM 2048

#define PTY_BUFFER_SIZE     32768


#define DEFAULT_FLAGS  (ConsoleManager::OPostProcess | \
                        ConsoleManager::IMapCRToNL | \
                        ConsoleManager::OMapNLToCRNL | \
                        ConsoleManager::LEcho | \
                        ConsoleManager::LEchoErase | \
                        ConsoleManager::LEchoKill | \
                        ConsoleManager::LCookedMode)

class ConsoleManager;
class ConsoleMasterFile;
class ConsoleSlaveFile;

class ConsoleFile : public File
{
    friend class ConsoleMasterFile;
    friend class ConsoleSlaveFile;
    friend class ConsoleManager;

    public:
        ConsoleFile(String consoleName, Filesystem *pFs);
        virtual ~ConsoleFile()
        {}

        virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true) = 0;
        virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true) = 0;

    protected:

        /// select - check and optionally for a particular state.
        int select(bool bWriting, int timeout);

        /// inject - inject bytes into the ring buffer
        void inject(char *buf, size_t len)
        {
            m_RingBuffer.write(buf, len);
            dataChanged();
        }

        /// Other side of the console.
        ConsoleFile *m_pOther;

        size_t m_Flags;

    private:

        RingBuffer<char> m_RingBuffer;
        String m_Name;

};

class ConsoleMasterFile : public ConsoleFile
{
    public:
        ConsoleMasterFile(String consoleName, Filesystem *pFs);
        virtual ~ConsoleMasterFile()
        {}

        virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
        virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);

        void setOther(ConsoleFile *pOther)
        {
            m_pOther = pOther;
            m_Flags = pOther->m_Flags;
        }

        /// Is this master locked (ie, already opened)?
        bool bLocked;

    private:

        /// Input line discipline
        void inputLineDiscipline(char *buf, size_t len);

        /// Output line discipline
        size_t outputLineDiscipline(char *buf, size_t len);

        /// Input line buffer.
        char m_LineBuffer[LINEBUFFER_MAXIMUM];

        /// Size of the input line buffer.
        size_t m_LineBufferSize;

        /// Location of the first newline in the line buffer. ~0 if none.
        size_t m_LineBufferFirstNewline;
};

class ConsoleSlaveFile : public ConsoleFile
{
    public:
        ConsoleSlaveFile(String consoleName, Filesystem *pFs);
        virtual ~ConsoleSlaveFile()
        {}

        virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
        virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);

        void setOther(ConsoleFile *pOther)
        {
            m_pOther = pOther;
        }

    private:

        /// Input processing.
        size_t processInput(char *buf, size_t len);
};

/** This class provides a way for consoles (TTYs) to be created to interact with applications.

    registerConsole is called by a class that subclasses RequestQueue. getConsole returns a File that forwards
    any read/write requests straight to that RequestQueue backend.

    read: request with p1: CONSOLE_READ, p2: param, p3: requested size, p4: buffer.
    write: request with p1: CONSOLE_WRITE, p2: param, p3: size of buffer, p4: buffer.
*/
class ConsoleManager : public Filesystem
{
public:

    enum IAttribute
    {
        IMapCRToNL        = 1,
        IIgnoreCR         = 2,
        IMapNLToCR        = 4,
        IStripToSevenBits = 8
    };
    enum OAttribute
    {
        OPostProcess     = 16,
        OMapCRToNL       = 32,
        ONoCrAtCol0      = 64,
        OMapNLToCRNL     = 128,
        ONLCausesCR      = 256
    };
    enum LAttribute
    {
        LEcho          = 512,
        LEchoErase     = 1024,
        LEchoKill      = 2048,
        LEchoNewline   = 4096,
        LCookedMode    = 8192
    };

    ConsoleManager();

    virtual ~ConsoleManager();

    static ConsoleManager &instance();

    //
    // ConsoleManager interface.
    //
    bool registerConsole(String consoleName, RequestQueue *backEnd, uintptr_t param);

    File* getConsole(String consoleName);
    ConsoleFile *getConsoleFile(RequestQueue *pBackend);

    /// Acquire a console master in such a way that it cannot be opened by another process.
    bool lockConsole(File *file);

    /// Release a console master locked as above.
    void unlockConsole(File *file);

    /// Create a new console - /dev/ptyXY -> /dev/ttyXY, where X is @c and Y is @i.
    void newConsole(char c, size_t i);

    bool isConsole(File* file);

    void setAttributes(File* file, size_t flags);
    void getAttributes(File* file, size_t *flags);
    int  getCols(File* file);
    int  getRows(File* file);
    bool hasDataAvailable(File* file);
    void flush(File *file);

    //
    // Filesystem interface.
    //

    virtual bool initialise(Disk *pDisk)
    {return false;}
    virtual File* getRoot()
    {return 0;}
    virtual String getVolumeLabel()
    {return String("consolemanager");}

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
    Vector<ConsoleFile*> m_Consoles;
    static ConsoleManager m_Instance;
    Spinlock m_Lock;
};

#endif
