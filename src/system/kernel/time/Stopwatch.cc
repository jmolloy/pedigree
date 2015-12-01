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

#include <time/Time.h>
#include <time/Stopwatch.h>

namespace Time
{

Stopwatch::Stopwatch(bool startRunning) :
    m_Value(), m_StartValue(), m_bRunning(startRunning)
{
    reset();

    if (m_bRunning)
        start();
}

Stopwatch::~Stopwatch()
{
}

void Stopwatch::start()
{
    m_bRunning = true;
    m_StartValue = Time::getTimeNanoseconds(false);
}

void Stopwatch::stop()
{
    m_bRunning = false;
    Timestamp now = Time::getTimeNanoseconds(false);
    m_Value += now - m_StartValue;
}

void Stopwatch::reset()
{
    m_Value = 0;
    m_StartValue = 0;
    m_bRunning = false;
}

Timestamp Stopwatch::value()
{
    return m_Value;
}

}
