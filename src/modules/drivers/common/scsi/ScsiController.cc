/*
 * Copyright (c) 2010 Eduard Burtescu
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

#include "ScsiController.h"
#include "ScsiDisk.h"

void ScsiController::searchDisks()
{
    for(size_t i = 0;i < getNumUnits();i++)
    {
        ScsiDisk *pDisk = new ScsiDisk();
        if(pDisk->initialise(this, i))
            addChild(pDisk);
        else
            delete pDisk;
    }
}

uint64_t ScsiController::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
    ScsiDisk *pDisk = reinterpret_cast<ScsiDisk*> (p2);
    if(p1 == SCSI_REQUEST_READ)
        return pDisk->doRead(p3);
    else if(p1 == SCSI_REQUEST_WRITE)
        return pDisk->doWrite(p3);
    else if(p1 == SCSI_REQUEST_SYNC)
        return pDisk->doSync(p3);
    else
        return 0;
}
