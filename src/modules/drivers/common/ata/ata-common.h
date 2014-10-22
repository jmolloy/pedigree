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

#ifndef _ATA_COMMON_H
#define _ATA_COMMON_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/IoBase.h>

typedef union
{
    /// This struct provides an easy way to literally code when working with
    /// the ATA status register.
    struct {
        uint32_t err : 1;
        uint32_t obs1 : 1;
        uint32_t obs2 : 1;
        uint32_t drq : 1;
        uint32_t rsvd1 : 1;
        uint32_t rsvd2 : 1;
        uint32_t drdy : 1;
        uint32_t bsy : 1;
    } PACKED reg;

    /// "Hidden" integer which contains the actual register contents
    uint8_t __reg_contents;
} AtaStatus;

typedef union
{
    struct {
        // Word 0: General Configuration
        struct {
            uint8_t              : 7;
            uint8_t is_removable : 1;
            uint8_t              : 7;
            uint8_t not_ata      : 1;
        } PACKED general_config;

        // Word 1: Obsolete
        uint16_t : 16;

        // Word 2: 'Specific Configuration'
        uint16_t : 16;

        // Word 3: Obsolete
        uint16_t : 16;

        // Words 4-5: Retired
        uint32_t : 32;

        // Word 6: Obsolete
        uint16_t : 16;

        // Words 7-8: Reserved for CompactFlash
        uint32_t : 32;

        // Word 9: Retired
        uint16_t : 16;

        // Words 10-19: Serial number
        char serial_number[20];

        // Words 20-21: Retired
        uint32_t : 32;

        // Word 22: Obsolete
        uint16_t : 16;

        // Words 23-26: Firmware revision
        char firmware_revision[8];

        // Words 27-46: Model number
        char model_number[40];

        // Word 47: Misc data.
        uint8_t always_80h;
        uint8_t max_sectors_per_irq;  // Max sectors per IRQ on READ/WRITE MULT.

        // Word 48: Reserved
        uint16_t : 16;

        // Word 49: 'Capabilities'
        struct {
            // Reserved
            uint8_t                 : 8;
            // DMA supported
            uint8_t dma             : 1;
            // LBA supported
            uint8_t lba             : 1;
            // IORDY may be disabled
            uint8_t iordy_disabled  : 1;
            // IORD supported
            uint8_t iordy_supported : 1;
            // Reserved for IDENTIFY PACKET DEVICE
            uint8_t                 : 1;
            // Standby timer values in standard are supported
            uint8_t std_standby     : 1;
            // Reserved for IDENTIFY PACKET DEVICE
            uint8_t                 : 2;
        } PACKED caps;

        // Word 50: 'Capabilities'
        uint16_t : 16;  // We ignore this field.

        // Words 51-52: Obsolete
        uint32_t : 32;

        // Word 53: 'Field Validity'
        struct {
            uint8_t                     : 1;
            uint8_t multiword_dma_valid : 1;
            uint8_t ultra_dma_valid     : 1;
            uint16_t                    : 13;
        } PACKED validity;

        // Words 54-58: Obsolete
        uint64_t : 64;
        uint16_t : 16;

        // Word 59: R/W MULTIPLE data
        struct {
            uint8_t curr_sectors_per_irq : 8;
            uint8_t is_valid             : 1;
            uint8_t                      : 7;
        } PACKED rwmultiple;

        // Words 60-61: LBA28 Addressable Sectors
        uint32_t sector_count;

        // Word 62: Obsolete
        uint16_t : 16;

        // Word 63: Multiword DMA information
        struct {
            uint8_t mode0       : 1;
            uint8_t mode1       : 1;
            uint8_t mode2       : 1;
            uint8_t             : 5;
            uint8_t sel_mode0   : 1;
            uint8_t sel_mode1   : 1;
            uint8_t sel_mode2   : 1;
            uint8_t             : 5;
        } PACKED multiword_dma;

        // Word 64: PIO mode support
        uint8_t pio_modes_supported;
        uint8_t  : 8;  // Reserved

        // Word 65: Minimum Multiword DMA transfer cycle time per word
        uint16_t minimum_multiword_dma_cycle;  // Nanoseconds

        // Word 66: Recommended Multiword DMA transfer cycle time
        uint16_t recommended_multiword_dma_cycle;  // Nanoseconds

        // Word 67: Minimum PIO transfer cycle time without flow control
        uint16_t minimum_pio_cycle;

        // Word 68: Minimum PIO transfer cycle time with IORDY flow control
        uint16_t minimum_pio_cycle_iordy;

        // Words 69-70: Reserved
        uint32_t : 32;

        // Words 71-74: Reserved for IDENTIFY PACKET DEVICE
        uint64_t : 64;

        // Word 75: 'Queue Depth'
        uint8_t max_queue_depth : 5;
        uint16_t                : 11;

        // Words 76-79: Reserved for Serial ATA
        uint64_t : 64;

        // Word 80: Major version number
        struct {
            uint8_t      : 4;
            uint8_t ata4 : 1;
            uint8_t ata5 : 1;
            uint8_t ata6 : 1;
            uint8_t ata7 : 1;
            // Reserved for future use (ATA/ATAPI-7 latest at time of writing)
            uint8_t      : 8;
        } PACKED major_version;

        // Word 81: Minor version number
        uint16_t : 16;

        // Word 82: 'Command Set Supported'
        struct {
            uint8_t smart        : 1;
            uint8_t security     : 1;
            uint8_t removable    : 1;
            uint8_t power        : 1;
            uint8_t packet       : 1;
            uint8_t cache        : 1;
            uint8_t lookahead    : 1;
            uint8_t release_int  : 1;
            uint8_t service_int  : 1;
            uint8_t device_reset : 1;
            uint8_t is_protected : 1;
            uint8_t              : 1;
            uint8_t write_buffer : 1;
            uint8_t read_buffer  : 1;
            uint8_t nop          : 1;
            uint8_t              : 1;
        } PACKED command_set_support;

        // Word 83: 'Command Sets Supported'
        struct {
            uint8_t microcode           : 1;
            uint8_t rw_dma_queued       : 1;
            uint8_t cfa                 : 1;
            uint8_t adv_power           : 1;
            uint8_t removable_notify    : 1;
            uint8_t powerup_standby     : 1;
            uint8_t set_features_spinup : 1;
            uint8_t                     : 1;
            uint8_t set_max             : 1;
            uint8_t acoustic            : 1;
            uint8_t address48           : 1;
            uint8_t config_overlay      : 1;
            uint8_t flush_cache         : 1;
            uint8_t flush_cache_ext     : 1;
            uint8_t one                 : 1;
            uint8_t zero                : 1;
        } PACKED command_sets_support;

        // Word 84: 'Command Set/Feature Supported Extension'
        struct {
            uint8_t smart_logging      : 1;
            uint8_t smart_selftest     : 1;
            uint8_t media_serial       : 1;
            uint8_t media_passthrough  : 1;
            uint8_t streaming          : 1;
            uint8_t logging            : 1;
            uint8_t write_mult_fua_ext : 1;
            uint8_t write_dma_fua_ext  : 1;
            uint8_t wwn_64             : 1;
            uint8_t read_dma_ext_urg   : 1;
            uint8_t write_dma_ext_urg  : 1;
            uint8_t                    : 2;
            uint8_t idle_immediate     : 1;
            uint8_t one                : 1;
            uint8_t zero               : 1;
        } PACKED command_set_ext_support;

        // Word 85: 'Command Set Enabled'
        struct {
            uint8_t smart        : 1;
            uint8_t security     : 1;
            uint8_t removable    : 1;
            uint8_t power        : 1;
            uint8_t packet       : 1;
            uint8_t cache        : 1;
            uint8_t lookahead    : 1;
            uint8_t release_int  : 1;
            uint8_t service_int  : 1;
            uint8_t device_reset : 1;
            uint8_t is_protected : 1;
            uint8_t              : 1;
            uint8_t write_buffer : 1;
            uint8_t read_buffer  : 1;
            uint8_t nop          : 1;
            uint8_t              : 1;
        } PACKED command_set_enabled;

        // Word 86: 'Command Sets Enabled'
        struct {
            uint8_t microcode           : 1;
            uint8_t rw_dma_queued       : 1;
            uint8_t cfa                 : 1;
            uint8_t adv_power           : 1;
            uint8_t removable_notify    : 1;
            uint8_t powerup_standby     : 1;
            uint8_t set_features_spinup : 1;
            uint8_t                     : 1;
            uint8_t set_max             : 1;
            uint8_t acoustic            : 1;
            uint8_t address48           : 1;
            uint8_t config_overlay      : 1;
            uint8_t flush_cache         : 1;
            uint8_t flush_cache_ext     : 1;
            uint8_t one                 : 1;
            uint8_t zero                : 1;
        } PACKED command_sets_enabled;

        // Word 87: 'Command Set/Feature Default'
        struct {
            uint8_t smart_logging      : 1;
            uint8_t smart_selftest     : 1;
            uint8_t media_serial       : 1;
            uint8_t media_passthrough  : 1;
            uint8_t streaming          : 1;
            uint8_t logging            : 1;
            uint8_t write_mult_fua_ext : 1;
            uint8_t write_dma_fua_ext  : 1;
            uint8_t wwn_64             : 1;
            uint8_t read_dma_ext_urg   : 1;
            uint8_t write_dma_ext_urg  : 1;
            uint8_t                    : 2;
            uint8_t idle_immediate     : 1;
            uint8_t one                : 1;
            uint8_t zero               : 1;
        } PACKED command_set_ext_default;

        // Word 88: Ultra DMA.
        struct {
            uint8_t supp_mode0 : 1;
            uint8_t supp_mode1 : 1;
            uint8_t supp_mode2 : 1;
            uint8_t supp_mode3 : 1;
            uint8_t supp_mode4 : 1;
            uint8_t supp_mode5 : 1;
            uint8_t supp_mode6 : 1;
            uint8_t            : 1;
            uint8_t sel_mode0  : 1;
            uint8_t sel_mode1  : 1;
            uint8_t sel_mode2  : 1;
            uint8_t sel_mode3  : 1;
            uint8_t sel_mode4  : 1;
            uint8_t sel_mode5  : 1;
            uint8_t sel_mode6  : 1;
            uint8_t            : 1;
        } PACKED ultra_dma;

        // Word 89: 'Time for security erase unit completion'
        uint16_t secure_erase_time;

        // Word 90: 'Time for enhanced security erase completion'
        uint16_t enhanced_secure_erase_time;

        // Word 91: 'Current Advanced Power Management Value'
        uint16_t curr_adv_power;

        // Word 92: 'Master Password Revision'
        uint16_t master_password_revision;

        // Word 93: 'Hardware reset result'
        struct {
            // Device 0
            struct {
                uint8_t one               : 1;
                // 00 = reserved, 01 = jumper, 10 = CSEL, 11 = unknown
                uint8_t method            : 2;
                uint8_t diagnostics       : 1;
                uint8_t pdiag_assert      : 1;
                uint8_t dasp_assert       : 1;
                // Responds when device 1 is selected.
                uint8_t responds_to_other : 1;
                uint8_t                   : 1;
            } PACKED device0;

            // Device 1
            uint8_t one1                  : 1;
            // 00 = reserved, 01 = jumper, 10 = CSEL, 11 = unknown
            uint8_t d1_method             : 2;
            uint8_t d1_pdiag_assert       : 1;
            uint8_t                       : 1;

            // General results
            uint8_t cbild_level           : 1;
            uint8_t one2                  : 1;
            uint8_t zero                  : 1;
        } PACKED hardware_reset;

        // Word 94: Vendor recommended and current acoustic management values.
        uint8_t current_acoustic;
        uint8_t vendor_acoustic;

        // Word 95: 'Stream Minimum Request Size'
        uint16_t stream_min_size;

        // Word 96: 'Streaming Transfer Time - DMA'
        uint16_t dma_streaming_time;

        // Word 97: 'Streaming Access Latency - DMA and PIO'
        uint16_t streaming_latency;

        // Words 98-99: 'Streaming Performance Granularity'
        uint32_t streaming_granularity;

        // Words 100-103: 'Maximum user LBA for 48-bit Address Feature set'
        uint64_t max_user_lba48;

        // Word 104: 'Streaming Transfer Time - PIO'
        uint16_t pio_streaming_time;

        // Word 105: Reserved
        uint16_t : 16;

        // Word 106: 'Physical Sector Size / Logical Sector Size'
        struct {
            // 2^X logical sectors per physical sector
            uint8_t logical_per_physical          : 4;
            uint8_t                               : 8;
            uint8_t logical_larger_than_512b      : 1;
            uint8_t multiple_logical_per_physical : 1;
            uint8_t one                           : 1;
            uint8_t zero                          : 1;
        } PACKED sector_size;

        // Word 107: 'Inter-seek Delay for ISO-9779 acoustic testing'
        uint16_t iso9779_delay;

        // Words 108-111: Identification
        struct {
            uint16_t oui1 : 12;
            uint8_t naa  : 4;
            uint8_t uid2 : 4;
            uint16_t oui0 : 12;
            uint16_t uid1;
            uint16_t uid0;
        } PACKED ident;

        // Words 112-115: Reserved for WWN extension to 128 bits.
        uint64_t : 64;

        // Word 116: Reserved.
        uint16_t : 16;

        // Word 117-118: 'Words per Logical Sector'
        uint32_t words_per_logical;

        // Words 119-126: Reserved
        uint64_t : 64;
        uint64_t : 64;

        // Word 127: 'Removable Media Status Notification Feature set support'
        uint16_t removable_notify_support;

        // Word 128: 'Security Status'
        struct {
            uint8_t supported             : 1;
            uint8_t enabled               : 1;
            uint8_t locked                : 1;
            uint8_t frozen                : 1;
            uint8_t count_expired         : 1;
            uint8_t ehanced_erase_support : 1;
            uint8_t                       : 2;
            uint8_t level                 : 1;
            uint8_t                       : 7;
        } PACKED security;

        // Words 129-159: 'Vendor Specific'
        uint16_t vendor[31];

        // Word 160: 'CFA Power Mode 1'
        uint16_t cfa_power_mode1;

        // Words 161-175: Reserved
        uint16_t rsvd[15];

        // Words 176-205: 'Current Media Serial Number'
        char media_serial[60];

        // Words 206-254: Reserved
        uint16_t rsvd2[49];

        // Word 255: 'Integrity Word'
        uint8_t signature;
        uint8_t checksum;
    } PACKED data;

    uint16_t __raw[256];
} IdentifyData;

/// Loads a buffer with N byte-swapped 16-bit words.
inline void ataLoadSwapped(char *out, char *in, size_t N)
{
    uint16_t *in16 = reinterpret_cast<uint16_t *>(in);
    for (size_t i = 0; i < N; ++i)
    {
#ifdef LITTLE_ENDIAN
        out[i * 2] = in16[i] >> 8;
        out[(i * 2) + 1] = in16[i] & 0xFF;
#else
        out[i * 2] = in16[i] & 0xFF;
        out[(i * 2) + 1] = in16[i] >> 8;
#endif
    }
}

/// Performs a proper wait for the drive to become ready as per the ATA spec
inline AtaStatus ataWait(IoBase *pBase)
{
    // Grab the status register first (of course)
    AtaStatus ret;
    uint8_t status = pBase->read8(7);
    if(status == 0)
    {
        ret.__reg_contents = status;
        return ret;
    }

    // Wait for BSY to be unset. Until BSY is unset, no other bits in the
    // register are considered valid.
    while(status & 0x80)
    {
        Processor::pause();
        status = pBase->read8(7);
    }

    // We no longer check DRQ as some commands depend on DRQ being set.
    // For example, a data transfer uses DRQ to say data is available for
    // reading.

    // And now verify that DRDY or ERR are asserted (or both!), but only
    // if DRQ is not set. DRDY will never be set if DRQ is set, and ERR
    // will come out in the return value even if DRQ is set.
    if(!(status & 0x8))
    {
        while(!(status & 0x41))
        {
            Processor::pause();
            status = pBase->read8(7);
            
            // If for some reason the original check for zero gave back something
            // set, we check again here so we don't infinitely loop waiting for
            // the bits that'll never be set. Essentially an escape for devices
            // that aren't present.
            if(!status)
                break;
        }
    }

    // Okay, BSY is unset now. The drive is no longer busy, it is up to the
    // caller to read the status and verify further bits (eg, ERR)
    ret.__reg_contents = status;
    return ret;
}

#endif
