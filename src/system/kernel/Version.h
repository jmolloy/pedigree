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

#ifndef VERSION_H
#define VERSION_H

/**
 * Ascii string giving the date and time of this build.
 */
extern const char *g_pBuildTime;

/**
 * Ascii string giving the SVN revision it was built from.
 */
extern const char *g_pBuildRevision;

/**
 * Ascii string giving the build flags used.
 */
extern const char *g_pBuildFlags;

/**
 * Ascii string giving the user who built us.
 */
extern const char *g_pBuildUser;

/**
 * Ascii string giving the machine we were built on.
 */
extern const char *g_pBuildMachine;

/**
 * Ascii string giving the target we were built for.
 */
extern const char *g_pBuildTarget;

#endif
