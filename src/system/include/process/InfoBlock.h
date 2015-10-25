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

#ifndef _INFO_BLOCK_H
#define _INFO_BLOCK_H

#ifdef __cplusplus

#include <processor/types.h>

#endif

struct InfoBlock
{
    /// Current timestamp in nanoseconds since the UNIX epoch.
    uint64_t now;

    /// Current process' ID.
    size_t pid;

    /// uname fields.
    char sysname[64];
    char release[64];
    char version[64];
    char machine[64];
};

#ifdef __cplusplus

#include <machine/TimerHandler.h>

class InfoBlockManager : public TimerHandler
{
public:
    InfoBlockManager();
    virtual ~InfoBlockManager();

    static InfoBlockManager &instance();

    bool initialise();

    virtual void timer(uint64_t delta, InterruptState &state);

    void setPid(size_t value);

private:
    static InfoBlockManager m_Instance;

    bool m_bInitialised;

    struct InfoBlock *m_pInfoBlock;

    uint64_t m_Ticks;
};


#endif


#endif
