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

#ifndef PROM_H
#define PROM_H

struct prom_args
{
  const char *service;
  int nargs;
  int nret;
  void *args[10];
};

typedef void *prom_handle;

typedef int (*prom_entry) (prom_args *);
void prom_init(prom_entry pe);

void *call_prom(const char *service, int nargs, int nret, void *a1=0, void *a2=0, void *a3=0, void *a4=0, void *a5=0, void *a6=0);

void *prom_finddevice(const char *dev);
int prom_exit();
int prom_getprop(prom_handle dev, char *name, void *buf, int len);
void prom_putchar(char c);
int prom_get_chosen(char *name, void *mem, int len);
void prom_map(unsigned int phys, unsigned int virt, unsigned int size);

#endif
