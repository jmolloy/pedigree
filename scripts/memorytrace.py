#!/usr/bin/env python

import os
import sys
import struct

unfreed = {}
counts = {}
toolarge = {}
total_allocs = 0
total_frees = 0

metadata = []

TOO_LARGE_THRESHOLD = 0x1000

MODULE_MAPPINGS = {
    'rawfs': 'build/modules/rawfs.o.debug',
    'iso9660': 'build/modules/iso9660.o.debug',
    'vmware-gfx': 'build/drivers/vmware-gfx.o.debug',
    'linker': 'build/modules/linker.o.debug',
    'ramfs': 'build/modules/ramfs.o.debug',
    'cdi': 'build/drivers/cdi.o.debug',
    'rtl8139': 'build/drivers/rtl8139.o.debug',
    'pci': 'build/drivers/pci.o.debug',
    'dhcpclient': 'build/modules/dhcpclient.o.debug',
    'dm9601': 'build/drivers/dm9601.o.debug',
    'usb-hub': 'build/drivers/usb-hub.o.debug',
    '3c90x': 'build/drivers/3c90x.o.debug',
    'network-stack': 'build/modules/network-stack.o.debug',
    'ext2': 'build/modules/ext2.o.debug',
    'loopback': 'build/drivers/loopback.o.debug',
    'status_server': 'build/modules/status_server.o.debug',
    'ps2mouse': 'build/drivers/ps2mouse.o.debug',
    'init': 'build/modules/init.o.debug',
    'splash': 'build/modules/splash.o.debug',
    'sis900': 'build/drivers/sis900.o.debug',
    'usb-mass-storage': 'build/drivers/usb-mass-storage.o.debug',
    'e1000': 'build/drivers/e1000.o.debug',
    'config': 'build/modules/config.o.debug',
    'preload': 'build/modules/preload.o.debug',
    'usb-hid': 'build/drivers/usb-hid.o.debug',
    'users': 'build/modules/users.o.debug',
    'nvidia': 'build/drivers/nvidia.o.debug',
    'gfx-deps': 'build/modules/gfx-deps.o.debug',
    'fat': 'build/modules/fat.o.debug',
    'ftdi': 'build/drivers/ftdi.o.debug',
    'ne2k': 'build/drivers/ne2k.o.debug',
    'hid': 'build/drivers/hid.o.debug',
    'lodisk': 'build/modules/lodisk.o.debug',
    'usb-hcd': 'build/drivers/usb-hcd.o.debug',
    'console': 'build/modules/console.o.debug',
    'usb': 'build/modules/usb.o.debug',
    'dma': 'build/drivers/dma.o.debug',
    'vbe': 'build/drivers/vbe.o.debug',
    'ata': 'build/drivers/ata.o.debug',
    'partition': 'build/drivers/partition.o.debug',
    'TUI': 'build/modules/TUI.o.debug',
    'pcnet': 'build/drivers/pcnet.o.debug',
    'scsi': 'build/drivers/scsi.o.debug',
    'vfs': 'build/modules/vfs.o.debug',
    'kernel': 'build/kernel/kernel.debug',
    'posix': 'build/subsystems/posix.o.debug',
    'native': 'build/subsystems/native.o.debug',
    'pedigree-c': 'build/subsystems/pedigree-c.o.debug',
}

class EarlyEof(Exception):
    pass

def saferead(f, sz):
    d = f.read(sz)
    if len(d) != sz:
        raise EarlyEof("early EOF hit (probably truncated file)")

    return d

def process_field(f):
    global unfreed, counts, toolarge, total_allocs, total_frees, metadata

    fieldtype, = saferead(f, 1)

    if fieldtype in ['A', 'F']:
        ptr, = struct.unpack('<L', saferead(f, 4)) # TODO: 64-bit

    if fieldtype == 'A':
        size, = struct.unpack('<L', saferead(f, 4)) # TODO: 64-bit

        backtrace = []
        while True:
            bt, = struct.unpack('<L', saferead(f, 4)) # TODO: 64-bit
            if bt == 0:
                break

            backtrace.append(bt)

        if size not in counts:
            counts[size] = (1, backtrace)
        else:
            c = counts[size]
            counts[size] = (c[0] + 1, c[1])

        if size >= TOO_LARGE_THRESHOLD:
            toolarge[ptr] = (size, backtrace)

        assert ptr not in unfreed
        unfreed[ptr] = (size, backtrace)

        total_allocs += 1
    elif fieldtype == 'F':
        try:
            del unfreed[ptr]
        except KeyError:
            pass # Possibly allocated before we had tracing active.

        total_frees += 1
    elif fieldtype == 'M':
        metaname, = struct.unpack('64s', saferead(f, 64))
        metaname = metaname.strip('\x00')
        start, = struct.unpack('<L', saferead(f, 4)) # TODO: 64-bit
        end, = struct.unpack('<L', saferead(f, 4)) # TODO: 64-bit
        metadata.append((metaname, start, end))
    else:
        print 'Invalid field type %r encountered at offset %d!' % (fieldtype, f.tell() - 5)
        sys.exit(1)

    return True

def addr2module(addr):
    for meta in metadata:
        name, low, high = meta
        if low <= addr < high:
            return low, MODULE_MAPPINGS.get(name, 'build/modules/%s.ko' % (name,))

    return 0, MODULE_MAPPINGS.get('kernel', 'build/kernel/kernel')

def writebt(bt, prefix=''):
    for frame in bt:
        # Figure out which module to use
        low, mod = addr2module(frame)
        if os.path.exists(mod):
            # Pretty-print, demangle, print function names.
            out = os.popen('addr2line -p -C -f -e %s 0x%x' % (mod, frame - low)).read()
            print '%s%s' % (prefix, out),
        else:
            print '%s%x (unknown module "%s")' % (prefix, frame, mod,)

    print

with open(sys.argv[1], 'rb') as f:
    try:
        while process_field(f):
            pass
    except EarlyEof:
        pass

    print "Loaded %d allocations and %d frees, and %d metadata items" % (total_allocs, total_frees, len(metadata))

    nobacktrace = 'nobacktrace' in sys.argv

    if 'unfreed' in sys.argv:
        print "Unfreed Blocks"

        total = 0
        for ptr, (size, backtrace) in unfreed.iteritems():
            total += size

            print "Unfreed %x of size %d:" % (ptr, size)
            if not nobacktrace:
                writebt(backtrace, '\t')

        print "Total %d bytes left unfreed." % (total)

    if 'counts' in sys.argv:
        print "Size Distribution"

        oneandover = 'oneandover' in sys.argv

        for size in sorted(counts.keys()):
            count, backtrace = counts[size]
            percent = (count / float(total_allocs)) * 100.0
            if oneandover and (percent < 1.0):
                continue

            print "%12d:\t%d\t(%f%%)" % (size, count, percent)
            if not nobacktrace:
                writebt(backtrace, '\t')

        print

    if 'toobig' in sys.argv:
        print "Too Big Blocks"

        for ptr, (size, backtrace) in toolarge.iteritems():
            print "Block %x should perhaps not be on the heap (it is %d bytes, larger than threshold %d):" % (ptr, size, TOO_LARGE_THRESHOLD)
            if not nobacktrace:
                writebt(backtrace, '\t')
