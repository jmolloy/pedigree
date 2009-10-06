/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include <Module.h>
#include <Log.h>

extern "C" {
    void init_pcnet(int argc, char *argv[]);
};

/// Wrapper for the actual driver's code
void pedigree_init_pcnet()
{
    init_pcnet(0, 0);
}

void pedigree_destroy_pcnet()
{
    ERROR("No MODULE_EXIT function call that we can use for pcnet");
}

MODULE_NAME("pcnet");
MODULE_ENTRY(&pedigree_init_pcnet);
MODULE_EXIT(&pedigree_destroy_pcnet);
MODULE_DEPENDS("cdi");
