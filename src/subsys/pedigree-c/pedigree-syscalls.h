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

#ifndef PEDIGREE_SYSCALLS_H
#define PEDIGREE_SYSCALLS_H

#include <compiler.h>

#ifndef PEDIGREE_SYSCALLS_LIBC

#ifdef PEDIGREEC_WITHIN_KERNEL

#include <processor/types.h>

#if 1
#define P_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "]\t" << Hex << x)
#else
#define P_NOTICE(x)
#endif

#endif

#endif

/** Pedigree configuration manager system calls **/

void pedigree_config_init();

void pedigree_config_getcolname(size_t resultIdx, size_t n, char *buf, size_t bufsz);
#ifndef __cplusplus
void pedigree_config_getstr_n(size_t resultIdx, size_t row, size_t n, char *buf, size_t bufsz);
void pedigree_config_getstr_s(size_t resultIdx, size_t row, const char *col, char *buf, size_t bufsz);
int pedigree_config_getnum_n(size_t resultIdx, size_t row, size_t n);
int pedigree_config_getnum_s(size_t resultIdx, size_t row, const char *col);
int pedigree_config_getbool_n(size_t resultIdx, size_t row, size_t n);
int pedigree_config_getbool_s(size_t resultIdx, size_t row, const char *col);
#else
void pedigree_config_getstr(size_t resultIdx, size_t row, size_t n, char *buf, size_t bufsz);
void pedigree_config_getstr(size_t resultIdx, size_t row, const char *col, char *buf, size_t bufsz);
int pedigree_config_getnum(size_t resultIdx, size_t row, size_t n);
int pedigree_config_getnum(size_t resultIdx, size_t row, const char *col);
int pedigree_config_getbool(size_t resultIdx, size_t row, size_t n);
int pedigree_config_getbool(size_t resultIdx, size_t row, const char *col);
#endif

int pedigree_config_query(const char *query);
void pedigree_config_freeresult(size_t resultIdx);
int pedigree_config_numcols(size_t resultIdx);
int pedigree_config_numrows(size_t resultIdx);

int pedigree_config_was_successful(size_t resultIdx);
void pedigree_config_get_error_message(size_t resultIdx, char *buf, int buflen);

/** Pedigree generic system calls **/

int pedigree_login(int uid, const char *password);

int pedigree_reboot();

void pedigree_module_load(char *file);
void pedigree_module_unload(char *name);
int pedigree_module_is_loaded(char *name);
int pedigree_module_get_depending(char *name, char *buf, size_t bufsz);

int pedigree_get_mount(char* mount_buf, char* info_buf, size_t n);

void *pedigree_sys_request_mem(size_t len);

void pedigree_haltfs();

/** Pedigree graphics framework system calls */

#ifdef __cplusplus
extern "C" {
#endif

void pedigree_input_install_callback(void *p, uint32_t type, uintptr_t param);
void pedigree_input_remove_callback(void *p);
int pedigree_load_keymap(char *buffer, size_t len);

void pedigree_event_return() NORETURN;

int pedigree_gfx_get_provider(void *p);
int pedigree_gfx_get_curr_mode(void *p, void *sm);
uintptr_t pedigree_gfx_get_raw_buffer(void *p);
int pedigree_gfx_create_buffer(void *p, void **b, void *args);
int pedigree_gfx_destroy_buffer(void *p, void *b);
int pedigree_gfx_convert_pixel(void *p, uint32_t *in, uint32_t *out, uint32_t infmt, uint32_t outfmt);
void pedigree_gfx_redraw(void *p, void *args);
void pedigree_gfx_blit(void *p, void *args);
void pedigree_gfx_set_pixel(void *p, uint32_t x, uint32_t y, uint32_t colour, uint32_t fmt);
void pedigree_gfx_rect(void *p, void *args);
void pedigree_gfx_copy(void *p, void *args);
void pedigree_gfx_line(void *p, void *args);
void pedigree_gfx_draw(void *p, void *args);

int pedigree_gfx_create_fbuffer(void *p, void *args);
void pedigree_gfx_delete_fbuffer(void *p);

void pedigree_gfx_fbinfo(void *p, size_t *w, size_t *h, uint32_t *fmt, size_t *bypp);
void pedigree_gfx_setpalette(void* p, uint32_t *data, size_t entries);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

