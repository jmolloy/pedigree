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

"""Repository license header fix script.

Fixes licensing for all files in the repository, EXCEPT those that are
specifically known to be third-party.

See LICENSE and CONTRIB for license and contributor information.
"""

import argparse
import difflib
import itertools
import logging
import os
import re


log = logging.getLogger(__name__)


LICENSE = 'LICENSE'
EXCLUDE = [
    './.git',
    './build',
    './compilers',
    './images',
    './pedigree-compiler',
    './src/lgpl',
    './src/system/kernel/machine/x86_common/x86emu',
    './src/subsys/posix/include',
    './src/modules/drivers/cdi',
    './src/user/applications/TUI/include',
]
EXTENSION_TRANSLATE = {
    '.cpp': '.cc',

    # Treat C as C++ when considering licensing comments.
    '.c': '.cc',

    # Treat header files as C++ for the same purpose.
    '.h': '.cc',

    # Pre-processed ASM.
    '.S': '.s',

    # NASM assembly.
    '.asm': '.s',
}


class SourceFile(object):
    """Base class representing a source file that can be licensed."""

    def __init__(self, filename, license):
        """Initialise internal state with raw license text, filename."""
        self.filename = filename
        self.license = license

        with open(filename) as f:
            self.filedata = f.read()

    def license_header(self):
        """Convert raw license text to a language-specific header."""
        raise NotImplementedError(
            'SourceFile subclasses must implement license_header!')

    def strip_existing(self):
        """Strip existing header (if any) from the file."""
        raise NotImplementedError()

    def check(self):
        """Check whether the file begins with the correct license header."""
        raise NotImplementedError()

    def fix(self):
        """Write back a fixed file, with correct licensing."""
        new_filedata = self.strip_existing()
        new_filedata = '%s\n%s' % (self.license_header(), new_filedata)

        log.debug('A diff of the changes that will be made is as follows:')
        diff = difflib.unified_diff(
            self.filedata.splitlines(), new_filedata.splitlines(), 'old', 'new')
        for line in diff:
            log.debug('    %s', line.strip())

        with open(self.filename, 'w') as f:
            f.write(new_filedata)


class PythonFile(SourceFile):
    """Methods for fixing licenses in Python files."""
    HEADER_RE = re.compile("^(\'\'\'.*?\'\'\')", re.DOTALL)

    def license_header(self):
        shebang = '#!/usr/bin/env python2.7\n'
        if not os.access(self.filename, os.X_OK):
            log.debug('Python script is not executable, not adding shebang.')
            shebang = ''  # Not executable.
        return "%s'''\n%s'''\n" % (shebang, self.license)

    def check(self):
        # Does the file even start with a multiline comment?
        if not self.filedata.startswith("'''"):
            return False

        # Does the file start with the exact license header?
        if not self.filedata.startswith(self.license_header()):
            return False

        return True

    def strip_existing(self):
        return_filedata = self.filedata

        # Kill off a shebang if it exists.
        if return_filedata.startswith('#!'):
            log.debug('Shebang line found, removing...')
            first_newline = return_filedata.find('\n')
            return_filedata = return_filedata[first_newline + 1:].lstrip()
            log.debug('Removed.')

        # Only strip an existing multiline comment if it's a license header.
        match = self.HEADER_RE.match(return_filedata)
        if match is None:
            log.debug('No multiline comment at start of file.')
            return return_filedata  # Already stripped.

        header = match.group(1)
        if 'Permission to use' in header:
            # This is a license header, and it must go.
            log.debug('Found an incorrect license header.')
            return_filedata = return_filedata[len(header) + 1:].lstrip()
        else:
            # This is not a license header. We should not remove it.
            pass

        return return_filedata


class CppFile(SourceFile):
    """Methods for fixing licenses in C/C++ source and header files."""
    HEADER_RE = re.compile('^(/\*.*?[ ]?\*/)', re.DOTALL)

    # Sign posts that identify an existing license header as NOT a
    # a Pedigree copyright.
    THIRDPARTY_SIGNPOSTS = [
        # WTFPL licensed code (we could theoretically relicense this)
        'WTFPL',
        # GPL (eek)
        'LGPL', 'GPL',
        # BSD licenses, especially those with the University clause
        'BSD', 'Berkeley', 'University',
        # 2-clause BSD doesn't talk about the university.
        'this list of conditions',
    ]

    def license_header(self):
        license_lines = self.license.splitlines()
        license_lines = [('\n * %s' % (x,)).rstrip() for x in license_lines]
        return '/*%s\n */\n' % (''.join(license_lines),)

    def check(self):
        # Does the file even start with a multiline comment?
        if not self.filedata.startswith('/*'):
            return False

        # Extra check (critically important for C/C++ files) to ensure
        # we do not accidentally destroy a third-party copyright.
        match = self.HEADER_RE.match(self.filedata)
        if match is not None:
            header = match.group(1).lower()
            if 'copyright' in header:
                # This is a copyright header, is it one of ours?
                for signpost in self.THIRDPARTY_SIGNPOSTS:
                    if signpost.lower() in header:
                        log.debug('Not a Pedigree license, not touching...')
                        log.debug('Hit the "%s" non-Pedigree signpost.',
                                  signpost)
                        return True  # Not a Pedigree license.

        # Does the file start with the exact license header?
        if not self.filedata.startswith(self.license_header()):
            return False

        return True

    def strip_existing(self):
        return_filedata = self.filedata

        # Only strip an existing multiline comment if it's a license header.
        match = self.HEADER_RE.match(return_filedata)
        if match is None:
            log.debug('No multiline comment at start of file.')
            return return_filedata  # Already stripped.

        header = match.group(1)
        if 'Permission to use' in header:
            # This is a license header, and it must go.
            log.debug('Found an incorrect license header.')
            return_filedata = return_filedata[len(header) + 1:].lstrip()
        else:
            # This is not a license header. We should not remove it.
            pass

        return return_filedata


class AsmFile(SourceFile):
    """Methods for fixing licenses in assembly (Intel, AT&T) files."""
    HEADER_INTEL_RE = re.compile('^((;.*[\r\n])+)', re.MULTILINE)
    HEADER_ATT_RE = re.compile('^((#.*[\r\n])+)', re.MULTILINE)

    REGISTER_RE = re.compile('([%]?)[re][abcd]x|[re][sd]i')

    ATT_SIGNPOSTS = ['.globl', '.global', '.extern', '.section']
    INTEL_SIGNPOSTS = ['[global', '[bits', '[section']

    ATT = 0
    INTEL = 1

    def __init__(self, filename, license):
        super(AsmFile, self).__init__(filename, license)

        self.filetype = -1

        # Generate hunt list for signposts.
        att = itertools.product([self.ATT], self.ATT_SIGNPOSTS)
        intel = itertools.product([self.INTEL], self.INTEL_SIGNPOSTS)
        self.hunt = itertools.chain(att, intel)

        # Detect ASM syntax, set license header method accordingly.
        with open(filename) as f:
            for l in f:
                if l.startswith('# '):
                    self.filetype = self.ATT
                    break
                elif l.startswith('; '):
                    self.filetype = self.INTEL
                    break

                l = l.lower()

                # No luck with comment lines, what about registers
                # or other specific syntax elements?
                s = self.REGISTER_RE.search(l)
                if s:
                    if s.group(1) == '%':
                        self.filetype = self.ATT
                    else:
                        self.filetype = self.INTEL

                    break

                for syntax, signpost in self.hunt:
                    if signpost in l:
                        log.debug('Syntax signpost found.')
                        self.filetype = syntax
                        break

                if self.filetype != -1:
                    break

        if self.filetype == -1:
            log.warning('Failed to detect ASM syntax for "%s"!', filename)
        elif self.filetype == self.ATT:
            self.license_header = self._license_header_att
        elif self.filetype == self.INTEL:
            self.license_header = self._license_header_intel

    def _license_header_att(self):
        license_lines = self.license.splitlines()
        return '# %s\n' % ('\n# '.join(license_lines),)

    def _license_header_intel(self):
        license_lines = self.license.splitlines()
        return '; %s\n' % ('\n; '.join(license_lines),)

    def _get_metadata(self):
        if self.filetype == self.ATT:
            return self.HEADER_ATT_RE, '#'
        elif self.filetype == self.INTEL:
            return self.HEADER_INTEL_RE, ';'

        return None, None

    def check(self):
        regex, commentchar = self._get_metadata()
        if regex is None:
            return True  # Heuristic failed, pretend all is fine.

        match = regex.match(self.filedata)

        # No comment at all to start the file.
        if match is None or not self.filedata.startswith(commentchar):
            return False

        # Do we have the exact license header anyway?
        if not self.filedata.startswith(self.license_header()):
            return False

        return True

    def strip_existing(self):
        regex, commentchar = self._get_metadata()
        if regex is None:
            return self.filedata  # Not sure what file type...

        match = regex.match(self.filedata)

        # No comment?
        if match is None or not self.filedata.startswith(commentchar):
            return self.filedata

        return_filedata = self.filedata

        # Kill off the existing comment only if it's a copyright header.
        header = match.group(1)
        if 'Permission to use' in header:
            # This is a license header. It must go.
            return_filedata = return_filedata[len(header) + 1:].lstrip()
        else:
            # Not a license header. Shouldn't be removed.
            pass

        return return_filedata


EXTENSIONS = {
    '.cc': CppFile,
    '.py': PythonFile,
    '.s': AsmFile,
    'SConscript': PythonFile,
    'SConstruct': PythonFile,
}


def checkfile(filename, license):
    """Check the given file for the given license.

    Will fix if the file needs fixing.
    """

    log.info('Processing "%s"...', filename)

    basename = os.path.basename(filename)

    _, ext = os.path.splitext(basename)
    if ext in EXTENSION_TRANSLATE:
        ext = EXTENSION_TRANSLATE.get(ext, ext)
        log.debug('Translated extension is "%s".', ext)

    cls = EXTENSIONS.get(basename)
    if cls is None:
        log.debug('Direct basename check failed, trying extension...')
        cls = EXTENSIONS.get(ext, SourceFile)

    if cls == SourceFile:
        log.info('Ignoring "%s" (unknown type).', basename)
    else:
        sourcefile = cls(filename, license)
        if not sourcefile.check():
            log.info('Header is incorrect, fixing...')
            sourcefile.fix()

            return True

    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--files', nargs='*',
                        help='Files to check and potentially fix. If not '
                        'passed, the full source tree will be walked.')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Be verbose about what is happening.')
    parser.add_argument('-l', '--license', help='Path to license template '
                        'file.', default=LICENSE)
    parser.add_argument('-o', '--output', nargs='?',
                        help='Path to output file to write logs to. '
                        'Implies -v.')
    parser.add_argument('-r', '--run', nargs='?',
                        help='Run the given command with each modified file '
                        'as a command line argument, upon successful fixing.')
    args = parser.parse_args()

    if args.verbose or args.output is not None:
        level = logging.DEBUG
    else:
        level = logging.INFO

    # Set up the logger.
    log_args = {'level': level, 'format': '%(asctime)s: %(message)s'}
    if args.output is not None:
        log_args['filename'] = args.output
        log_args['filemode'] = 'w'
    logging.basicConfig(**log_args)

    fullscriptpath = os.path.realpath(__file__)
    scriptdir = os.path.dirname(fullscriptpath)

    with open(args.license) as f:
        license = f.read()

    if args.files is None:
        # Make sure we're at the root of the source tree, so walked paths
        # end up relative to the root (which helps exclusions).
        olddir = os.getcwd()
        os.chdir(scriptdir)

        dirs = set([])
        for root, dirlist, _ in os.walk('.'):
            for dirname in dirlist:
                fullpath = os.path.join(root, dirname)
                for exclude in EXCLUDE:
                    if fullpath.startswith(exclude):
                        break
                else:
                    dirs.add(fullpath)

        for path in dirs:
            for filename in os.listdir(path):
                fullpath = os.path.join(path, filename)

                if not os.path.isfile(fullpath):
                    continue

                result = checkfile(fullpath, license)
                if result and args.run is not None:
                    os.system('%s %s' % (args.run, fullpath))

        # Restore old working directory.
        os.chdir(olddir)
    else:
        for thefile in args.files:
            if not os.path.isfile(thefile):
                log.debug('Ignoring non-file "%s"...', thefile)
                continue

            result = checkfile(thefile, license)
            if result and args.run is not None:
                os.system('%s %s' % (args.run, thefile))


if __name__ == '__main__':
    main()


