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

# Pre-PLT functions - space usage: 17 of 18 words (68 of 72 bytes).

plt_resolve:
  mr 14, 3        # 0 -- r3, r4 and r5 get trashed by the syscall, so move to temporary registers.
  mr 15, 4        # 1
  mr 16, 5        # 2

  addi 3, 0, 1    # 3 -- 1 = KernelCoreSyscallManager::link

  addis 4, 0, 0   # 4 -- These will be filled in by the pltInit function
  addi 4, 4, 0    # 5 -- to be a library identifier.

  mr 5, 11        # 6 -- PLT index.
  sc              # 7

  mtctr 12        # 8 -- Return value should be branched to.

  mr 5, 16        # 9
  mr 4, 15        # 10
  mr 6, 14        # 11
  bctr            # 12

plt_call:
  addis 11, 11, 0 # 13 -- These get filled in by the pltInit function
  lwz 11, 0(11)   # 14 -- to be the address of the PLT table.
  mtctr 11        # 15
  bctr            # 16

  nop             # 17
  nop             # 18
