#!/usr/bin/env python2.7
'''
Copyright (c) 2008-2014, Pedigree Developers

Please see the CONTRIB file in the root of the source tree for a full
list of contributors.

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
'''

"""Tests that a build of Pedigree has been successful by running Pedigree.

Runs QEMU and monitors the serial port for a particular string. Only works if
TRAVIS=true, as this is what triggers the write of the string to the serial
port.
"""

import os
import signal
import socket
import subprocess
import sys
import time


class TimeoutError(Exception):
    pass


class Timeout(object):
    def __init__(self, seconds=1):
        self.seconds = seconds

    def handle_timeout(self, signum, frame):
        raise TimeoutError('timed out')

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        signal.alarm(0)


def main(argv):
    travis = os.environ.get('TRAVIS')
    if not travis:
        print 'Not running on Travis, aborting.'
        return 1

    target = os.environ.get('EASY_BUILD')
    if not target:
        print 'Bad Travis configuration - $EASY_BUILD not set.'
        return 1
    elif target != 'x64':
        print 'Tests only run on x86-64 currently.'
        return 0

    scriptdir = os.path.dirname(os.path.realpath(__file__))

    # Open UDP socket for the serial port.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('127.0.0.1', 4556))

    # Disable default serial ports in the QEMU script.
    os.environ['NO_SERIAL_PORTS'] = 'yes'

    # Kick off the QEMU instance in the background.
    qemu_cmd = [
        os.path.join(scriptdir, 'qemu'),
        '-m',
        '512',
        '-nographic',
        '-serial',
        'udp:127.0.0.1:4556',
        '-monitor',
        'stdio'
    ]

    start = time.time()
    qemu = subprocess.Popen(qemu_cmd, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            close_fds=True, cwd=os.path.join(scriptdir, '..'))

    # Skip over GRUB boot menu.
    time.sleep(1)
    qemu.stdin.write('sendkey ret\n')

    success = False
    serial = []
    try:
        with Timeout(60):
            last = ''
            while True:
                serial_data = last + sock.recv(1024)
                serial_lines = serial_data.split('\n')
                last = serial_lines.pop()

                serial.extend(serial_lines)

                if '-- Hello, Travis! --' in '\n'.join(serial_lines):
                    success = True
                    break
        end = time.time()
    except TimeoutError:
        print 'Runtime test failure: Pedigree did not boot to the login prompt.'
        print 'Most recent serial lines:'
        print '\n'.join(serial[-15:])
    else:
        print ('Runtime test success: Pedigree booted to the login '
               'prompt (%ds).' % (int(end - start),))

    # Terminate QEMU now.
    qemu.communicate('quit\n')

    # Serial socket is done - QEMU is no more.
    sock.close()

    # Were we successful?
    return 0 if success else 1


if __name__ == '__main__':
    exit(main(sys.argv))
