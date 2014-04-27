/*
 * 
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define PASS 1
#define FAIL 0

int test_open(void)
{
  printf ("open():\n");
  printf ("\tOpen existing file - ");
  int fd = open ("/applications/shell", O_RDONLY);
  if (fd != -1)
  {
    close(fd);
    printf("PASS\n");
  }
  else
  {
    printf("FAIL - errno %d (%s)\n", errno, strerror(errno));
  }

  printf ("\tOpen nonexistant file - ");
  int fd2 = open ("/applications/penis", O_RDONLY);
  if (fd2 == -1 && errno == ENOENT)
  {
    close(fd2);
    printf("PASS\n");
  }
  else
  {
    printf ("FAIL - errno %d (%s)\n", errno, strerror(errno));
  }

  printf ("\tCreate file - ");
  int fd3 = open ("/file-doesnt-exist", O_RDWR | O_CREAT);
  if (fd3 != -1)
  {
    close(fd3);
    printf("PASS\n");
  }
  else
  {
    printf("FAIL - errno %d (%s)\n", errno, strerror(errno));
  }

  printf("\tRecycle descriptors - ");
  int fd4 = open ("/applications/bash", O_RDWR);
  close(fd4);
  int fd5 = open ("/applications/bash", O_RDWR);
  close(fd5);
  if(fd4 == fd5)
  {
    printf("PASS\n");
  }
  else
  {
    printf("FAIL - %d, %d\n", fd4, fd5);
  }

  int hahaha = open("/applications/bash", O_RDWR);
  if(fork() == 0)
  {
      close(hahaha);
      int rofl = open("/applications/bash", O_RDWR);

      printf("%d/%d\n", hahaha, rofl);

      close(rofl);
      exit(0);
  }
  close(hahaha);

  printf("Complete\n");

  while(1);

  return PASS;
}

int main (int argc, char **argv)
{
  printf("argc: %d, argv[0]: %s, &optind: %x\n", argc, argv[0], &optind);
  while(
    getopt(argc, argv,
       "h?ABC:DEFHIKLNOQ:RST:UVWY:abcdefgijklmo:pr:s:tvwxz") != -1)
  {printf("bleh\n");}
  printf("optind: %d\n", optind);
  printf ("Syscall test starting...\n");

  test_open ();
  return 0;

}
