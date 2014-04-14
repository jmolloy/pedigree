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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <signal.h>

volatile int some_global = 0;

void rofl(int arg)
{
    printf("Signal Handler (arg=%x)!\n", arg);
    exit(2);
}

int main(int argc, char **argv)
{
    /*  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
      if(sock == -1)
      {
        printf("Couldn't get a socket: %d [%s]\n", errno, strerror(errno));
        return 1;
      }

      struct timeval t;
      t.tv_sec = 30;

      fd_set readfd;
      FD_SET(sock, &readfd);

      char* tmp = (char*) malloc(2048);
      while(1)
      {
        select(sock + 1, &readfd, 0, 0, &t);
        int n = read(sock, tmp, 2048);
        if(n > 0)
          printf("interface received %d bytes\n", n);
      }
    */

    printf("Installing signal handler...\n");

    signal(SIGINT, rofl);

    printf("CTRL-C should break this loop\n");
    while(1);

    return 0;
}
