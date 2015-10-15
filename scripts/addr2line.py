#!/usr/bin/env python
from __future__ import print_function

# Wrapper script to perform addr2line, using data from the serial log.

import os
import operator
import re
import sys


def parse_serial(path):
    with open(path) as f:
        lines = f.readlines()

    # Extract modules
    modules = []

    exp = re.compile('Preloaded module (.*) at 0x([0-9a-fA-F]*) to 0x([0-9a-fA-F]*)')

    for line in lines:
        match = exp.search(line)
        if not match:
            continue

        modules.append((match.group(1), (int(match.group(2), 16),
                        int(match.group(3), 16))))

    modules = sorted(modules, key=operator.itemgetter(1))

    return dict(modules)


def main():
    scriptdir = os.path.dirname(os.path.realpath(__file__))

    try:
        addresses = [int(x, 16) for x in sys.argv[2:]]
    except ValueError:
        addresses = []
    if not addresses:
        print('No addresses specified.')
        return 1

    # TODO(miselin): allow this to work with custom build directories.
    build_dir = os.path.join(scriptdir, '..', 'build')

    try:
        serial_file = sys.argv[1]
    except IndexError:
        print('No serial file given.')
        return 1

    kernel_dir = os.path.join(build_dir, 'kernel')
    if not os.path.isdir(kernel_dir):
        print('No kernel directory exists, cannot run.')
        return 1

    modules_dir = os.path.join(build_dir, 'modules')
    drivers_dir = os.path.join(build_dir, 'drivers')
    subsystems_dir = os.path.join(build_dir, 'subsystems')

    modules = parse_serial(serial_file)

    for address in addresses:
        which = None
        for module, (module_start, module_end) in modules.items():
            if module_start <= address < module_end:
                address -= module_start
                which = module
                break

        addend = 0x30  # .text offset in modules. This is horrible.
        if not which:
            which = 'kernel'
            addend = 0

        for d in (kernel_dir, modules_dir, drivers_dir, subsystems_dir):
            filename = os.path.join(d, '%s.o.debug' % which)
            if not os.path.isfile(filename):
                filename = os.path.join(d, '%s.debug' % which)
                if not os.path.isfile(filename):
                    continue

            os.system('addr2line -p -C -f -e %s 0x%x' % (filename, address + addend))


    return 0

if __name__ == '__main__':
    exit(main())
