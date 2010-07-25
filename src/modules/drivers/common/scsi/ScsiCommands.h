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
    class Inquiry : public ScsiCommand
    {
        public:
            inline Inquiry(uint16_t len = 0, bool enableVitalData = false, uint8_t pageCode = 0, uint8_t ctl = 0)
            {
                memset(&command, 0, sizeof(command));
                command.opcode = 0x12;
                command.epvd = enableVitalData;
                if(enableVitalData)
                    command.pageCode = pageCode;
                command.len = HOST_TO_BIG16(len);
                command.control = ctl;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct
            {
                uint8_t opcode;
                uint8_t epvd;
                uint8_t pageCode;
                uint16_t len;
                uint8_t control;
            } PACKED command;
    };

    class UnitReady : public ScsiCommand
    {
        public:
            inline UnitReady(uint8_t ctl = 0)
            {
                memset(&command, 0, sizeof(command));
                command.opcode = 0;
                command.control = ctl;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct
            {
                uint8_t opcode;
                uint32_t rsvd;
                uint8_t control;
            } PACKED command;
    };

    class ReadSense : public ScsiCommand
    {
        public:
            inline ReadSense(uint8_t desc, uint8_t len, uint8_t ctl = 0)
            {
                memset(&command, 0, sizeof(command));
                command.opcode = 0x03;
                command.desc = desc;
                command.len = len;
                command.control = ctl;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct
            {
                uint8_t opcode;
                uint8_t desc;
                uint16_t rsvd;
                uint8_t len;
                uint8_t control;
            } PACKED command;
    };

    class StartStop : public ScsiCommand
    {
        public:
            inline StartStop(bool imm, uint8_t newpower, bool eject_load, bool start, uint8_t ctl = 0)
            {
                memset(&command, 0, sizeof(command));
                command.opcode = 0x1b;
                command.imm = imm ? 1 : 0;
                command.setup = (start ? 1 : 0) | ((eject_load ? 1 : 0) << 1) | (newpower << 4);
                command.control = ctl;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct
            {
                uint8_t opcode;
                uint8_t imm;
                uint16_t rsvd;
                uint8_t setup;
                uint8_t control;
            } PACKED command;
    };

    class SendDiagnostic : public ScsiCommand
    {
        public:
            inline SendDiagnostic(bool selfTest, uint8_t selfTestCode = 0, uintptr_t params = 0, size_t paramLen = 0, bool deviceOffline = false, bool unitOffline = false, uint8_t ctl = 0)
            {
                memset(&command, 0, sizeof(command));
                command.opcode = 0x1d;
                command.unitOffline = unitOffline;
                command.devOffline = deviceOffline;
                command.selfTest = selfTest;
                command.pf = 0;
                command.selfTestCode = selfTestCode;
                command.paramListLen = HOST_TO_BIG16(paramLen);
                command.control = ctl;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct
            {
                uint8_t opcode;
                uint32_t unitOffline : 1;
                uint32_t devOffline : 1;
                uint32_t selfTest : 1;
                uint32_t rsvd1 : 1;
                uint32_t pf : 1;
                uint32_t selfTestCode : 3;
                uint8_t rsvd2;
                uint16_t paramListLen;
                uint8_t control;
            } PACKED command;
    };

    class ReadCapacity10 : public ScsiCommand
    {
        public:
            inline ReadCapacity10(uint8_t ctl = 0)
            {
                memset(&command, 0, sizeof(command));
                command.opcode = 0x25;
                command.control = ctl;
            }

            virtual size_t serialise(uintptr_t &addr)
            {
                addr = reinterpret_cast<uintptr_t>(&command);
                return sizeof(command);
            }

            struct
            {
                uint8_t opcode;
                uint8_t obsolete_rsvd;
                uint32_t lba;
                uint8_t rsvd[2];
                uint8_t pmi;
                uint8_t control;
            } PACKED command;
    };

    class Read10 : public ScsiCommand
    {
        public:
            inline Read10(uint32_t nLba, uint32_t nSectors)
            {
                memset(&command, 0, sizeof(command));
                command.nOpCode = 0x28;
                command.nLba = HOST_TO_BIG32(nLba);
                command.nSectors = HOST_TO_BIG16(nSectors);
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
                uint16_t nSectors;
                uint8_t nControl;
            } PACKED command;
    };

    class Read12 : public ScsiCommand
    {
        public:
            inline Read12(uint32_t nLba, uint32_t nSectors)
            {
                memset(&command, 0, sizeof(command));
                command.nOpCode = 0xa8;
                command.nLba = HOST_TO_BIG32(nLba);
                command.nSectors = HOST_TO_BIG32(nSectors);
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
                uint32_t nSectors;
                uint8_t res2;
                uint8_t nControl;
            } PACKED command;
    };

    class Read16 : public ScsiCommand
    {
        public:
            inline Read16(uint32_t nLba, uint32_t nSectors)
            {
                memset(&command, 0, sizeof(command));
                command.nOpCode = 0x88;
                command.nLba = HOST_TO_BIG64(nLba);
                command.nSectors = HOST_TO_BIG32(nSectors);
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
                uint32_t nSectors;
                uint8_t res2;
                uint8_t nControl;
            } PACKED command;
    };
};

#endif
