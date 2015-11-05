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

#ifndef SYS_DIRENT_H
#define SYS_DIRENT_H

#include <sys/types.h>

#define MAXNAMLEN 255

#define DT_UNKNOWN      0
#define DT_REG          1
#define DT_DIR          2
#define DT_FIFO         3
#define DT_SOCK         4
#define DT_CHR          5
#define DT_BLK          6
#define DT_LNK          7

struct dirent
{
  char d_name[MAXNAMLEN];
  ino_t d_ino;
  int d_type;
};

typedef struct ___DIR
{
  int fd;
  int pos;
  int totalpos;
  int count;
  struct dirent ent[64];
} DIR;


#define __dirfd(dir) ((dir)->fd)

#ifdef __cplusplus
extern "C" 
{
#endif

DIR *opendir(const char*);
struct dirent *readdir(DIR *);
void rewinddir(DIR *);
int closedir(DIR *);

int alphasort(const struct dirent **d1, const struct dirent **d2);
int scandir(const char *dir, struct dirent ***namelist,
       int (*sel)(const struct dirent *),
       int (*compar)(const struct dirent **, const struct dirent **));

#ifdef __cplusplus
}
#endif

#endif
