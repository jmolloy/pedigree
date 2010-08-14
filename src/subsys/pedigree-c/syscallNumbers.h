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

#ifndef SYSCALL_NUMBERS_H
#define SYSCALL_NUMBERS_H

#define PEDIGREE_LOGIN                      1
#define PEDIGREE_SIGRET                     2
#define PEDIGREE_INIT_SIGRET                3
#define PEDIGREE_INIT_PTHREADS              4
#define PEDIGREE_LOAD_KEYMAP                5
#define PEDIGREE_GET_MOUNT                  6
#define PEDIGREE_REBOOT                     7

#define PEDIGREE_CONFIG_GETCOLNAME          8
#define PEDIGREE_CONFIG_GETSTR_N            9
#define PEDIGREE_CONFIG_GETNUM_N            10
#define PEDIGREE_CONFIG_GETBOOL_N           11
#define PEDIGREE_CONFIG_GETSTR_S            12
#define PEDIGREE_CONFIG_GETNUM_S            13
#define PEDIGREE_CONFIG_GETBOOL_S           14
#define PEDIGREE_CONFIG_QUERY               15
#define PEDIGREE_CONFIG_FREERESULT          16
#define PEDIGREE_CONFIG_NUMCOLS             17
#define PEDIGREE_CONFIG_NUMROWS             18
#define PEDIGREE_CONFIG_NEXTROW             19
#define PEDIGREE_CONFIG_WAS_SUCCESSFUL      20
#define PEDIGREE_CONFIG_GET_ERROR_MESSAGE   21

#define PEDIGREE_MODULE_LOAD                22
#define PEDIGREE_MODULE_UNLOAD              23
#define PEDIGREE_MODULE_IS_LOADED           24
#define PEDIGREE_MODULE_GET_DEPENDING       25

#define PEDIGREE_GFX_GET_PROVIDER           64
#define PEDIGREE_GFX_GET_CURR_MODE          65
#define PEDIGREE_GFX_GET_RAW_BUFFER         66
#define PEDIGREE_GFX_CREATE_BUFFER          67
#define PEDIGREE_GFX_DESTROY_BUFFER         68
#define PEDIGREE_GFX_REDRAW                 69
#define PEDIGREE_GFX_BLIT                   70
#define PEDIGREE_GFX_RECT                   71
#define PEDIGREE_GFX_COPY                   72
#define PEDIGREE_GFX_LINE                   73
#define PEDIGREE_GFX_SET_PIXEL              74

#endif

