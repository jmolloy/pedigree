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

#ifndef SCSICONTROLLER_H
#define SCSICONTROLLER_H

#include <processor/types.h>
#include <machine/Controller.h>
#include <utilities/RequestQueue.h>

#define SCSI_REQUEST_READ       1
#define SCSI_REQUEST_WRITE      2
#define SCSI_REQUEST_SYNC       3

/** Generic class for Scsi Controllers */
class ScsiController: public Controller, public RequestQueue
{
    public:
        ScsiController(Controller *pDev);
        ScsiController();

        virtual ~ScsiController(){}

        virtual bool sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite) =0;

        virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                        uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8);

    protected:

        virtual size_t getNumUnits() =0;

        void searchDisks();
};

#endif
