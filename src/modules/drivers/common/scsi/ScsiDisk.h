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

#ifndef SCSIDISK_H
#define SCSIDISK_H

#include <processor/types.h>
#include <machine/Disk.h>
#include <utilities/Cache.h>
#include "ScsiCommands.h"

class ScsiDisk : public Disk
{
    private:
        enum ScsiPeripheralType
        {
            BlockDevice         = 0x00,
            SequentialDevice    = 0x01,
            PrinterDevice       = 0x02,
            ProcessorDevice     = 0x03,
            WriteOnceDevice     = 0x04,
            CdDvdDevice         = 0x05,
            OpticalMemoryDevice = 0x07,
            MediumChangerDevice = 0x08,
            ArrayDevice         = 0x0C,
            EnclosureDevice     = 0x0D,
            NoDevice            = 0x1F,
        };
    public:
        struct Sense
        {
            uint8_t ResponseCode;
            uint8_t Obsolete;
            uint32_t SenseKey : 4;
            uint32_t rsvd : 1;
            uint32_t ili : 1;
            uint32_t eom : 1;
            uint32_t filemark : 1;
            uint32_t info;
            uint8_t addLen;
            uint32_t commandInfo;
            uint8_t Asc;
            uint8_t AscQ;
            uint8_t fieldCode;
            uint8_t senseKeySpecific[3];
        } PACKED;

        struct Inquiry
        {
            uint8_t Peripheral;
            uint8_t Removable;
            uint8_t Version;
            uint8_t Flags1;
            uint8_t AddLength;
            uint8_t Rsvd[2];
            uint8_t Flags2;
            uint8_t VendIdent[8];
            uint8_t ProdIdent[16];
            uint8_t ProdRev[4];
        } PACKED;

        struct Capacity
        {
            uint32_t LBA;
            uint32_t BlockSize;
        } PACKED;

        ScsiDisk();
        virtual ~ScsiDisk();

        bool initialise(class ScsiController* pController, size_t nUnit);

        virtual uintptr_t read(uint64_t location);
        virtual void write(uint64_t location);
        virtual void flush(uint64_t location);
        virtual void align(uint64_t location);

        virtual void getName(String &str)
        {
            str = String("SCSI Disk");
        }

        // These are the internal functions that the controller calls when it is ready to process our request.
        virtual uint64_t doRead(uint64_t location);
        virtual uint64_t doWrite(uint64_t location);
        virtual uint64_t doSync(uint64_t location);

        virtual size_t getSize() const
        {
            return m_NumBlocks * m_NativeBlockSize;
        }

        virtual size_t getBlockCount() const
        {
            return m_NumBlocks;
        }

        virtual size_t getBlockSize() const
        {
            return m_BlockSize;
        }

        /**
         * Retrieves the native block size - that is, the logical block size.
         * This differs from the main block size, which is for caching.
         */
        virtual size_t getNativeBlockSize() const
        {
            return m_NativeBlockSize;
        }

        virtual void pin(uint64_t location);
        virtual void unpin(uint64_t location);

    protected:
        Cache &getCache()
        {
            return m_Cache;
        }

        Inquiry *getInquiry() const
        {
            return m_Inquiry;
        }

    private:

        bool unitReady();

        bool readSense(Sense *s);

        bool sendCommand(ScsiCommand *pCommand, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite=false);

        bool getCapacityInternal(size_t *blockNumber, size_t *blockSize);

        static void cacheCallback(Cache::CallbackCause cause, uintptr_t loc, uintptr_t page, void *meta);

        class ScsiController* m_pController;
        size_t m_nUnit;

        Cache m_Cache;
        Inquiry *m_Inquiry;

        uint64_t m_AlignPoints[8];
        size_t m_nAlignPoints;

        size_t m_NumBlocks;
        size_t m_BlockSize;
        size_t m_NativeBlockSize;

        ScsiPeripheralType m_DeviceType;

        /** Default block size for a device */
        virtual size_t defaultBlockSize()
        {
            return m_BlockSize;
        }
};

#endif
