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

#include <BootstrapInfo.h>
#include <utilities/utility.h>
#include <Log.h>
#include "DiskImage.h"

#include <sys/stat.h>
#include <sys/mman.h>

extern BootstrapStruct_t *g_pBootstrapInfo;

DiskImage::DiskImage(const char *path) : Disk(), m_pFileName(path), m_nSize(0),
    m_pFile(0), m_pBuffer(0)
{
}

DiskImage::~DiskImage()
{
    if (m_pBuffer)
    {
        msync(m_pBuffer, m_nSize, MS_SYNC);
        munmap(m_pBuffer, m_nSize);
    }

    if (m_pFile)
    {
        fflush(m_pFile);
        fclose(m_pFile);
    }
}

bool DiskImage::initialise()
{
    if (m_pFile)
        return false;

    m_pFile = fopen(m_pFileName, "rb+");
    if (!m_pFile)
        return false;

    int fd = fileno(m_pFile);
    struct stat st;
    int r = fstat(fd, &st);
    if (r < 0)
    {
        fclose(m_pFile);
        m_pFile = 0;
        return false;
    }

    m_nSize = st.st_size;

    m_pBuffer = mmap(0, m_nSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (m_pBuffer == MAP_FAILED)
    {
        fclose(m_pFile);
        m_pFile = 0;
        return false;
    }

    posix_madvise(m_pBuffer, m_nSize, POSIX_MADV_SEQUENTIAL);

    return true;
}

uintptr_t DiskImage::read(uint64_t location)
{
    if((location > m_nSize) || !m_pFile)
    {
        return ~0;
    }

    return reinterpret_cast<uintptr_t>(m_pBuffer) + location;
}

void DiskImage::write(uint64_t location)
{
    if((location > m_nSize) || !m_pFile)
    {
        return;
    }

    msync(adjust_pointer(m_pBuffer, location), getBlockSize(), MS_ASYNC);
}

size_t DiskImage::getSize() const
{
    return m_nSize;
}

void DiskImage::pin(uint64_t location)
{
}

void DiskImage::unpin(uint64_t location)
{
}
