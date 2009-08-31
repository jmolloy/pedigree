/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, Eduard Burtescu
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
#ifndef USB_CONSTANTS_H
#define USB_CONSTANTS_H

#include <compiler.h>
#include <processor/types.h>

#define HCI_CLASS       0x0C
#define HCI_SUBCLASS    0x03
#define HCI_PROGIF      0x20

typedef struct UsbSetup
{
    uint8_t req_type;
    uint8_t req;
    uint16_t val;
    uint16_t index;
    uint16_t len;
} PACKED UsbSetup;

typedef struct UsbDeviceDescriptor
{
    uint8_t len;
    uint8_t descriptor;
    uint16_t bcd_usb;
    uint8_t dev_class;
    uint8_t dev_subclass;
    uint8_t dev_proto;
    uint8_t max_pack;
    uint16_t ven_id;
    uint16_t prod_id;
    uint16_t bcd_dev;
    uint8_t str_ven;
    uint8_t str_prod;
    uint8_t str_devsn;
    uint8_t nconf;
} PACKED UsbDeviceDescriptor;

typedef struct UsbConfig
{
    uint8_t len;
    uint8_t descriptor;
    uint16_t total_len;
    uint8_t ifaces;
    uint8_t config;
    uint8_t str;
    uint8_t attr;
    uint8_t max_pow;
} PACKED UsbConfig;

typedef struct UsbInterface
{
    uint8_t len;
    uint8_t descriptor;
    uint8_t iface;
    uint8_t alt_setting;
    uint8_t nend;
    uint8_t iface_class;
    uint8_t iface_subclass;
    uint8_t iface_proto;
    uint8_t str;
} PACKED UsbInterface;

typedef struct UsbInfo
{
    UsbDeviceDescriptor *dev;
    UsbConfig *conf;
    UsbInterface *iface;

    char *str_prod;
    char *str_ven;
    char *str_conf;
    char *str_iface;
    uint8_t usb_class;
    uint8_t usb_subclass;
    uint8_t usb_protocol;
} UsbInfo;

enum UsbConstants {
    USB_PID_SETUP = 0x2d,   // Setup PID
    USB_PID_IN = 0x69,      // In PID
    USB_PID_OUT = 0xe1,     // Out PID
};

enum UHCIConstants {
    UHCI_CMD = 0x00,             // Command register
    UHCI_STS = 0x02,             // Status register
    UHCI_INTR = 0x04,            // Intrerrupt Enable register
    UHCI_FRMN = 0x06,            // Frame Number register
    UHCI_FRLP = 0x08,            // Frame List Pointer register

    UHCI_CMD_DBG = 0x20,         // Debug command
    UHCI_CMD_GRES = 0x04,        // Global Reset command
    UHCI_CMD_HCRES = 0x02,       // Host Controller Reset command
    UHCI_CMD_RUN = 0x01,         // Run command

    UHCI_BUFF_SIZE = 0x10000,    // The size of the Rx and Tx buffers

    UHCI_PACK_MAX = 0x10000,     // The maximal size of a packet
    UHCI_PACK_MIN = 0x16,        // The minimal size of a packet
};

#endif
