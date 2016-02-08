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

import os
import subprocess
import sys
import unittest


class Ext2Tests(unittest.TestCase):
    pass


def generate_new_test(ext2img, script, should_pass):
    """Generate a test that runs ext2img to complete."""
    def _setup(self):
        # Pre-test: create the image.
        with open('_t.img', 'w') as f:
            f.truncate(0x1000000)
        subprocess.check_call(['/sbin/mke2fs', '-q', '-O', '^dir_index', '-I',
            '128', '-F', '-L', 'pedigree', '_t.img'], stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)

    def call(self, wrapper=None):
        try:
            _setup(self)
        except:
            self.skipTest('cannot create image for test, skipping')

        args = [ext2img, '-c', script, '-f', '_t.img']
        if wrapper:
            args = wrapper + args
        result = subprocess.call(args)

        if should_pass:
            self.assertEqual(result, 0)

            # Make sure an fsck passes too, now that we've checked the
            # invocation itself passed.
            args = ['/sbin/fsck.ext2', '-n', '-f', '_t.img']
            fsck_result = subprocess.call(args)
            self.assertEqual(fsck_result, 0)
        else:
            self.assertNotEqual(result, 0)

    def test_doer(self):
        call(self)

    def test_memcheck_doer(self):
        call(self, wrapper=['/usr/bin/valgrind', '--tool=memcheck',
            '--error-exitcode=1'])

    def test_sgcheck_doer(self):
        call(self, wrapper=['/usr/bin/valgrind', '--tool=exp-sgcheck',
            '--error-exitcode=1'])

    testname = os.path.basename(script).replace('.test', '').replace('.', '_')

    returns = (
        test_doer,
        # test_memcheck_doer,
        # test_sgcheck_doer,
    )

    for r in returns:
        r.__name__ = r.__name__.replace('doer', testname)

    return returns


def find_pedigree_tests():
    # Should be run from the top level of the source tree.
    ext2img_bin = 'build/host/ext2img/ext2img'
    testdir = 'tests/ext2'

    # Find tests to run.
    for f in os.listdir(testdir):
        f = os.path.join(testdir, f)

        # Each .test file is a command list for ext2img. The first line of a
        # .test file needs to indicate (in a comment) whether this invocation
        # should succeed or fail.
        if not f.endswith('.test'):
            continue

        with open(f) as f_:
            start = f_.read(64).splitlines()[0]
            should_pass = 'pass' in start.lower()

        tests = generate_new_test(ext2img_bin, f, should_pass)
        for test in tests:
            setattr(Ext2Tests, test.__name__, test)


find_pedigree_tests()
