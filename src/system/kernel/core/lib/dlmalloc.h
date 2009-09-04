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

#ifndef DLMALLOC_H
#define DLMALLOC_H

#include <processor/types.h>
#include <utilities/utility.h>
#define ABORT dlmallocAbort
#define HAVE_MORECORE 1
#define MORECORE dlmallocSbrk
#define MORECORE_CANNOT_TRIM 1
#define MORECORE_CONTIGUOUS 1
#define HAVE_MMAP 0
#define MALLOC_FAILURE_ACTION

#define LACKS_UNISTD_H 1
#define LACKS_FCNTL_H 1
#define LACKS_SYS_PARAM_H 1
#define LACKS_SYS_MMAN_H 1
#define LACKS_STRINGS_H 1
#define LACKS_STRING_H 1
#define LACKS_SYS_TYPES_H
#define LACKS_ERRNO_H
#define LACKS_STDLIB_H

#define DEBUG 1
#define FOOTERS 1

#ifdef __cplusplus
  extern "C"
  {
#endif
#define MALLINFO_FIELD_TYPE size_t
      struct mallinfo {
          MALLINFO_FIELD_TYPE arena;    /* non-mmapped space allocated from system */
          MALLINFO_FIELD_TYPE ordblks;  /* number of free chunks */
          MALLINFO_FIELD_TYPE smblks;   /* always 0 */
          MALLINFO_FIELD_TYPE hblks;    /* always 0 */
          MALLINFO_FIELD_TYPE hblkhd;   /* space in mmapped regions */
          MALLINFO_FIELD_TYPE usmblks;  /* maximum total allocated space */
          MALLINFO_FIELD_TYPE fsmblks;  /* always 0 */
          MALLINFO_FIELD_TYPE uordblks; /* total allocated space */
          MALLINFO_FIELD_TYPE fordblks; /* total free space */
          MALLINFO_FIELD_TYPE keepcost; /* releasable (via malloc_trim) space */
      };


    //
    // Functions exported from dlmalloc
    //
    void *malloc(size_t);
    void free(void *);
    struct mallinfo mallinfo();

    //
    // Functions needed for dlmalloc
    //
    void dlmallocAbort(void);
    void *dlmallocSbrk(ssize_t incr);

//    void dlmallocAssertFailed(const char *x, const char *line);

#ifdef __cplusplus
  }
#endif

#endif
