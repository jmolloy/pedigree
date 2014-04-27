# Copyright (c) 2008-2014, Pedigree Developers
# 
# Please see the CONTRIB file in the root of the source tree for a full
# list of contributors.
# 
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

.global sigret_stub
.global sigret_stub_end

# void sigret_stub(size_t p1, size_t p2, size_t p3, size_t p4)#
sigret_stub:

    # TODO: write
    b .

sigret_stub_end:

.global pthread_stub
.global pthread_stub_end
.extern pthread_exit

# void pthread_stub(size_t p1, size_t p2, size_t p3, size_t p4)#
pthread_stub:

    # TODO: write
    b .

pthread_stub_end:
