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

#ifndef SCSICONTROLLER_H
#define SCSICONTROLLER_H

#include <processor/types.h>
#include <machine/Controller.h>

/** Generic class for Scsi Controllers */
class ScsiController: public Controller
{
    public:

        inline ScsiController(){}
        inline virtual ~ScsiController(){}

        virtual bool sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite) =0;

    protected:

        virtual size_t getNumUnits() =0;

        void searchDisks();
};

#endif
