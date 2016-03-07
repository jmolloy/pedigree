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

    def tearDown(self):
        # Clean up the disk image now that we're done.
        if os.path.exists('_t.img'):
            os.unlink('_t.img')


def generate_new_test(ext2img, script, should_pass, sz=0x1000000, suffix=None):
    """Generate a test that runs ext2img to complete."""
    def _setup(self):
        # Pre-test: create the image.
        with open('_t.img', 'w') as f:
            f.truncate(sz)
        subprocess.check_call(['/sbin/mke2fs', '-q', '-O', '^dir_index', '-I',
                               '128', '-F', '-L', 'pedigree', '_t.img'],
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def call(self, wrapper=None):
        try:
            _setup(self)
        except:
            self.skipTest('cannot create image for test, skipping')

        args = [ext2img, '-c', script, '-f', '_t.img']
        if wrapper:
            args = wrapper + args
        result = subprocess.Popen(args, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)

        run_result = result.wait()
        run_output, _ = result.communicate()

        if should_pass:
            self.assertEqual(run_result, 0, 'exit status %d != 0\n'
                             'output:\n\n%s\n' % (
                                 run_result, run_output))

            # Make sure an fsck passes too, now that we've checked the
            # invocation itself passed.
            args = ['/sbin/fsck.ext2', '-n', '-f', '_t.img']
            fsck = subprocess.Popen(args, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            fsck_result = fsck.wait()
            fsck_output, _ = fsck.communicate()
            self.assertEqual(fsck_result, 0,
                             msg='exit status %d != 0\nfsck output:\n\n%s\n'
                                 'ext2img output:\n%s\n' % (fsck_result,
                                                            fsck_output,
                                                            run_output))
        else:
            self.assertNotEqual(run_result, 0, 'exit status %d == 0\n'
                                'ext2img output:\n%s\n' % (run_result,
                                                           run_output))

    def test_doer(self):
        call(self)

    def test_memcheck_doer(self):
        call(self, wrapper=['/usr/bin/valgrind', '--tool=memcheck',
                            '--error-exitcode=1'])

    def test_sgcheck_doer(self):
        call(self, wrapper=['/usr/bin/valgrind', '--tool=exp-sgcheck',
                            '--error-exitcode=1'])

    testname = os.path.basename(script).replace('.test', '').replace('.', '_')
    if suffix is not None:
        testname += '.%s' % (suffix,)

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

            # Allow temporary disables for tests
            if 'disable' in start.lower():
                continue

        for sz in (0x100000 * 16, 0x100000 * 256, 0x100000 * 512):
            tests = generate_new_test(ext2img_bin, f, should_pass, sz=sz,
                                      suffix='%dMB' % (sz / 0x100000,))
            for test in tests:
                setattr(Ext2Tests, test.__name__, test)


find_pedigree_tests()
