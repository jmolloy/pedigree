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

#include <utilities/RequestQueue.h>

RequestQueue::RequestQueue() :
  m_pRequestQueue(0), m_RequestQueueSize(0), m_Stop(false)
{
}

RequestQueue::~RequestQueue()
{
}

void RequestQueue::initialise()
{
}

void RequestQueue::destroy()
{
}

uintptr_t RequestQueue::addRequest(uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4,
                                   uintptr_t p5, uintptr_t p6, uintptr_t p7, uintptr_t p8)
{
}

int RequestQueue::trampoline(void *p)
{
  RequestQueue *pRQ = reinterpret_cast<RequestQueue*> (p);
  return pRQ->work();
}

int RequestQueue::work()
{
}
