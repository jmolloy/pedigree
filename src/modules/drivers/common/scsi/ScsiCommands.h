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

#ifndef SCSICOMMANDS_H
#define SCSICOMMANDS_H

#include <processor/types.h>
#include <utilities/utility.h>

class ScsiCommand
{
    public:
        inline ScsiCommand() {}
        virtual inline ~ScsiCommand() {}

        virtual size_t serialise(uintptr_t &addr) = 0;
};

namespace ScsiCommands
{
    class Read10 : public ScsiCommand
    {
        public:
            inline Read10(uint32_t lba, uint32_t length)
            {
                memset(&command, 0, sizeof(command));
                command.nOpCode = 0x28;
                command.nLba = HOST_TO_BIG32(lba);
                command.nLength = HOST_TO_BIG16(length);
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct command
            {
                uint8_t nOpCode;
                uint8_t bRelAddr : 1;
                uint8_t res0 : 2;
                uint8_t bFUA : 1;
                uint8_t bDPO : 1;
                uint8_t res1 : 3;
                uint32_t nLba;
                uint8_t res2;
                uint16_t nLength;
                uint8_t nControl;
            } PACKED command;
    };

    class Read12 : public ScsiCommand
    {
        public:
            inline Read12(uint32_t lba, uint32_t length)
            {
                memset(&command, 0, sizeof(command));
                command.nOpCode = 0xa8;
                command.nLba = lba;
                command.nLength = length;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct command
            {
                uint8_t nOpCode;
                uint8_t bRelAddr : 1;
                uint8_t res0 : 2;
                uint8_t bFUA : 1;
                uint8_t bDPO : 1;
                uint8_t res1 : 3;
                uint32_t nLba;
                uint32_t nLength;
                uint8_t res2;
                uint8_t nControl;
            } PACKED command;
    };

    class Read16 : public ScsiCommand
    {
        public:
            inline Read16(uint32_t lba, uint32_t length)
            {
                memset(&command, 0, sizeof(command));
                command.nOpCode = 0x88;
                command.nLba = lba;
                command.nLength = length;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct command
            {
                uint8_t nOpCode;
                uint8_t bRelAddr : 1;
                uint8_t res0 : 2;
                uint8_t bFUA : 1;
                uint8_t bDPO : 1;
                uint8_t res1 : 3;
                uint64_t nLba;
                uint32_t nLength;
                uint8_t res2;
                uint8_t nControl;
            } PACKED command;
    };
};

#endif
