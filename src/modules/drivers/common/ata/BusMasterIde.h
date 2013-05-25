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

#ifndef BUSMASTERIDE_H
#define BUSMASTERIDE_H

#include <processor/types.h>
#include <processor/IoBase.h>
#include <processor/IoPort.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>

/**
 * If set to 1, the BusMasterIde object will log fairly verbosely (outcome of
 * each DMA transfer).
 */
#define BUSMASTER_VERBOSE_LOGGING 0

/** The Physical Region Descriptor structure */
typedef struct
{
    // First bit = zero
    uint32_t physAddr;

    // First bit = zero
    uint16_t byteCount;

    // Last bit = EOT
    uint16_t rsvdEot;
} __attribute__((packed)) PhysicalRegionDescriptor;

/** Command register format */
typedef union
{
    uint8_t reg;

    /// \todo I blanked when writing this, so I can't remember if this defines
    ///       "bits" as a packed structure or if the attribute is ignored...
    struct
    {
        uint8_t startStop   : 1;
        uint8_t rsvd1       : 2;
        uint8_t rwCtl       : 1;
        uint8_t rsvd2       : 4;
    } __attribute__((packed)) bits;
} BusMasterIdeCommandRegister;

/** Status register format */
typedef union
{
    uint8_t reg;
    struct bits
    {
        uint8_t bmIdeActive : 1;
        uint8_t error       : 1;
        uint8_t interrupt   : 1;
        uint8_t rsvd : 2;
        uint8_t drv0Capable : 1;
        uint8_t drv1Capable : 1;
        uint8_t simplexOnly : 1;
    } __attribute__((packed)) bits;
} BusMasterIdeStatusRegister;

/** Descriptor table pointer register */
typedef struct
{
    /// Base address of the table, first two bits must be zero
    uint32_t baseAddress;
} BusMasterIdeDescriptorTablePointerRegister;

/** Generic Bus Master IDE object. Given to devices that can use DMA by their
 *  controller, this class provides a set of routines for setting up and
 *  completing DMA transactions in conjunction with other devices on the
 *  same channel.
 *  The idea is to hide away as much of the gritty implementation of IDE DMA
 *  so that drivers have a clean and easy interface to use when performing DMA
 *  operations. This should also reduce code duplication.
 */
class BusMasterIde
{
    public:
        BusMasterIde();
        virtual ~BusMasterIde();

        /** \brief Initialises DMA for a specific channel.
         *  \param pBase The IO base for this channel. This is, for example,
         *               BAR4 when working with PIIX's IDE interface.
         *  \return True if the channel is set up and ready, false if not.
         *
         *  In the off chance the given IO base is not usable (null, wrong
         *  ports/mmio addresses, for example), being able to return false
         *  means controllers can detect a failure condition and avoid giving
         *  devices DMA information.
         */
        bool initialise(IoBase *pBase);

        /** \brief Adds a buffer to a DMA transaction.
         *  \param buffer Address of the buffer to be used for the transaction
         *  \param nBytes Size of the buffer
         *  \param bWrite Whether or not this is a write operation
         *  \return True if setting up the transaction is successful, false
         *          otherwise.
         *
         * This will add a buffer to an existing transaction, or begin a new
         * one if no transaction already exists. Calling @commandComplete
         * will remove all pending buffer (as they are complete).
         */
        bool add(uintptr_t buffer, size_t nBytes);

        /** \brief Begin a DMA operation.
         *  \param bWrite Whether or not this is a write operation.
         *  \return True if beginning the transaction is successful, false
         *          otherwise.
         *
         *  Driver code calls this to begin an operation.
         *  This call will store the buffer, and will begin the Bus Master
         *  operation if it is not already in progress. For every buffer,
         *  device driver software should send a command to the respective
         *  device.
         */
        bool begin(bool bWrite);

        /** \brief Determines if an INTERRUPT has occurred on this channel.
         *  \return True if an interrupt has occurred, false otherwise.
         *
         *  When writing driver code that uses a DMA transaction, waiting on
         *  an IRQ is the normal way to wait for the transaction to complete.
         *  This function call allows driver code to determine if an interrupt
         *  has been triggered on the DMA channel in order to filter spurious
         *  interrupts coming in.
         */
        bool hasInterrupt();

        /** \brief Determines if an ERROR has occurred on this channel.
         *  \return True if an error has occurred, false otherwise.
         *
         *  If the interrupt status has been raised, it is imperative for
         *  driver software to check if an error occurred in their transfer.
         *  The specific error can be obtained through a device-specific
         *  method; this merely provides notification that the transfer failed
         */
        bool hasError();

        /** \brief Determines if the CURRENT TRANSFER has completed.
         *  \return True if the transfer has completed, false otherwise.
         *
         *  Some bus mastering controllers don't provide an IRQ line and others
         *  may need this particular function to be waited upon to be sure that
         *  the transfer has completed even if an IRQ is fired.
         */
        bool hasCompleted();

        /** \brief Called by drivers when a command completes
         *
         *  The interface requires that we reset the command register's start
         *  bit upon command completion or failure. However, we don't want to
         *  do that in hasInterrupt or hasError because that may leave the
         *  interface in an unknown state. This function allows driver code to
         *  reset this bit at a convenient time.
         *
         *  \note This effectively resets the interface. That means the return
         *        values from hasInterrupt and hasError will both be invalid
         *        after calling this function.
         */
        void commandComplete();
    private:
        /** Internal I/O base */
        IoBase *m_pBase;

        /** PRD table lock. This is used when inserting items to the PRDT */
        Mutex m_PrdTableLock;

        /** PRD table (virtual) */
        PhysicalRegionDescriptor *m_PrdTable;

        /** Last used offset into the PRD table (so we can run multiple ops at once) */
        size_t m_LastPrdTableOffset;

        /** PRD table (physical) */
        physical_uintptr_t m_PrdTablePhys;

        /** MemoryRegion for the PRD table */
        MemoryRegion m_PrdTableMemRegion;

        /** Register layout */
        enum BusMasterIdeRegs
        {
            Command         = 0,    // R/W
            DeviceSpecific1 = 1,    // --
            Status          = 2,    // R/W/C
            DeviceSpecific2 = 3,    // --
            PrdTableAddr    = 4     // R/W
        };
};

#endif
