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

#ifndef USBDESCRIPTORS_H
#define USBDESCRIPTORS_H

#include <processor/types.h>

struct UsbEndpointDescriptor
{
    uint8_t nLength;
    uint8_t nType;
    uint8_t nEndpoint : 4;
    uint8_t res0 : 3;
    uint8_t bDirection : 1;
    uint8_t nTransferType : 2;
    uint8_t res1 : 6;
    uint16_t nMaxPacketSize : 11;
    uint8_t res2 : 5;
    uint8_t nInterval;
} PACKED;

struct UsbInterfaceDescriptor
{
    uint8_t nLength;
    uint8_t nType;
    uint8_t nInterface;
    uint8_t nAlternateSetting;
    uint8_t nEndpoints;
    uint8_t nClass;
    uint8_t nSubclass;
    uint8_t nProtocol;
    uint8_t nString;
} PACKED;

struct UsbConfigurationDescriptor
{
    uint8_t nLength;
    uint8_t nType;
    uint16_t nTotalLength;
    uint8_t nInterfaces;
    uint8_t nConfig;
    uint8_t nString;
    uint8_t nAttributes;
    uint8_t nMaxPower;
} PACKED;

struct UsbDeviceDescriptor
{
    uint8_t nLength;
    uint8_t nType;
    uint16_t nBcdUsbRelease;
    uint8_t nClass;
    uint8_t nSubclass;
    uint8_t nProtocol;
    uint8_t nMaxControlPacketSize;
    uint16_t nVendorId;
    uint16_t nProductId;
    uint16_t nBcdDeviceRelease;
    uint8_t nVendorString;
    uint8_t nProductString;
    uint8_t nSerialString;
    uint8_t nConfigurations;
} PACKED;

#endif
