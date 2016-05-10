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

#include <machine/Machine.h>
#include <Log.h>

class SerialLogger : public Log::LogCallback
{
    public:
        void callback(const char* str)
        {
            if (!Machine::instance().isInitialised())
                return;

            for(size_t n = 0; n < Machine::instance().getNumSerial(); n++)
            {
#if defined(MEMORY_TRACING) || (defined(MEMORY_LOGGING_ENABLED) && !defined(MEMORY_LOG_INLINE)) || defined(INSTRUMENTATION)
                if(n == 1) // Don't override memory log.
                    continue;
#endif

                Machine::instance().getSerial(n)->write(str);
#ifndef SERIAL_IS_FILE
                // Handle carriage return if we're writing to a real terminal
                // Technically this will create a \n\r, but it will do the same
                // thing. This may also be redundant, but better to be safe than
                // sorry imho.
                Machine::instance().getSerial(n)->write('\r');
#endif
            }
        }
};

static SerialLogger g_SerialCallback;

void installSerialLogger()
{
    Log::instance().installCallback(&g_SerialCallback, false);
}
