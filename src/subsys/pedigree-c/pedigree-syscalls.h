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

#ifndef PEDIGREE_SYSCALLS_H
#define PEDIGREE_SYSCALLS_H

#include <processor/types.h>

#if 1
#define P_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "]\t" << Hex << x)
#else
#define P_NOTICE(x)
#endif

/** Pedigree configuration manager system calls **/

void pedigree_config_init();

void pedigree_config_getcolname(size_t resultIdx, size_t n, char *buf, size_t bufsz);
void pedigree_config_getstr(size_t resultIdx, size_t n, char *buf, size_t bufsz);
void pedigree_config_getstr(size_t resultIdx, const char *col, char *buf, size_t bufsz);
int pedigree_config_getnum(size_t resultIdx, size_t n);
int pedigree_config_getnum(size_t resultIdx, const char *col);
int pedigree_config_getbool(size_t resultIdx, size_t n);
int pedigree_config_getbool(size_t resultIdx, const char *col);

int pedigree_config_query(const char *query);
void pedigree_config_freeresult(size_t resultIdx);
int pedigree_config_numcols(size_t resultIdx);
int pedigree_config_numrows(size_t resultIdx);
int pedigree_config_nextrow(size_t resultIdx);

int pedigree_config_was_successful(size_t resultIdx);
void pedigree_config_get_error_message(size_t resultIdx, char *buf, int buflen);

/** Pedigree generic system calls **/

int pedigree_login(int uid, const char *password);

int pedigree_load_keymap(char *buffer, size_t len);
int pedigree_reboot();

int pedigree_module_load(char *file);
int pedigree_module_is_loaded(char *name);

int pedigree_get_mount(char* mount_buf, char* info_buf, size_t n);

#endif
