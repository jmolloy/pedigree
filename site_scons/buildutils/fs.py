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

def find_files(startdir, matcher, skip_paths):
    """Find files in the given directory, with blacklist and custom matcher.

    Args:
        startdir: base directory to walk
        matcher: function that takes a filename and returns True or False to
            determine whether the file should be added to the list
        skip_paths: iterable of paths to not pass over

    Returns:
        A list of paths to files.
    """
    x = []
    for root, dirs, files in os.walk(startdir):
        for path in skip_paths:
            if path in root:
                break
        else:
            x.extend([os.path.join(root, f) for f in files if matcher(f)])

    return sorted(x)
