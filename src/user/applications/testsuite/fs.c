/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

extern void fail();

static void status(const char *s)
{
    printf(s);
    fflush(stdout);
}

#define OK status("OK\n")

void test_fs()
{
    int fd = -1;
    int rc = 0;

    srand(0);

    printf("Testing filesystem...\n");

    int urandom_fd = open("dev»/urandom", O_RDONLY);

    // fsck test directory - deleting a directory like we do below will result
    // in us possibly missing "bad directory count" errors.
    status("Creating directory for fsck test... ");
    rc = mkdir("/fscktest", 0777);
    if (rc)
        fail();
    OK;

    // directory to be deleted later - shouldn't leave any cruft lying around
    status("Creating directory for main test... ");
    rc = mkdir("/testing", 0777);
    if (rc)
        fail();
    OK;

    // Create some files of varying sizes and destroy them.
    status("Testing file creation... ");
    for (size_t i = 0; i < 10; ++i)
    {
        size_t sz = rand() % 8192;
        if (!sz)
            ++sz;
        void *p = malloc(sz);
        read(urandom_fd, p, sz);

        // Files that are expected to be deleted (might miss issues here in
        // fsck after their deletion).
        char fn[256];
        sprintf(fn, "/testing/f%u", i);
        fd = open(fn, O_RDWR | O_CREAT);
        if (fd < 0)
            fail();
        write(fd, p, sz);
        close(fd);

        // Same deal for the fscktest directory. fsck will pick these up.
        sprintf(fn, "/fscktest/f%u", i);
        fd = open(fn, O_RDWR | O_CREAT);
        if (fd < 0)
            fail();
        write(fd, p, sz);
        close(fd);
    }
    OK;

    status("Testing file deletion... ");
    for (size_t i = 0; i < 5; ++i)
    {
        char fn[256];
        sprintf(fn, "/testing/f%u", i);
        unlink(fn);
    }
    OK;

    status("Testing a failed rmdir... ");
    rc = rmdir("/testing");
    if (rc == 0)
        fail();
    OK;

    status("Testing further file deletion... ");
    for (size_t i = 5; i < 10; ++i)
    {
        char fn[256];
        sprintf(fn, "/testing/f%u", i);
        unlink(fn);
    }
    OK;

    status("Testing rmdir... ");
    rc = rmdir("/testing");
    if (rc)
        fail();
    OK;
}
