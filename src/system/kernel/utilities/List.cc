/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include <utilities/List.h>

//
// List<void*> implementation
//

List<void*>::List()
  : m_Count(0), m_First(0), m_Last(0)
{
}
List<void*>::List(const List<void*> &x)
  : m_Count(0), m_First(0), m_Last(0)
{
  assign(x);
}
List<void*>::~List()
{
  clear();
}

List<void*> &List<void*>::operator = (const List<void*> &x)
{
  assign(x);
  return *this;
}

size_t List<void*>::size() const
{
  return m_Count;
}
size_t List<void*>::count() const
{
  return m_Count;
}
void List<void*>::pushBack(void *value)
{
  node_t *newNode = new node_t;
  newNode->next = 0;
  newNode->previous = m_Last;
  newNode->value = value;

  if (m_Last == 0)
    m_First = newNode;
  else
    m_Last->next = newNode;

  m_Last = newNode;
  ++m_Count;
}
void *List<void*>::popBack()
{
  node_t *node = m_Last;
  m_Last = m_Last->previous;
  m_Last->next = 0;
  --m_Count;

  void *value = node->value;
  delete node;
  return value;
}
void List<void*>::pushFront(void *value)
{
  node_t *newNode = new node_t;
  newNode->next = m_First;
  newNode->previous = 0;
  newNode->value = value;

  if (m_First == 0)
    m_Last = newNode;
  else
    m_First->previous = newNode;

  m_First = newNode;
  ++m_Count;
}
void *List<void*>::popFront()
{
  node_t *node = m_First;
  m_First = m_First->next;
  m_First->previous = 0;
  --m_Count;

  void *value = node->value;
  delete node;
  return value;
}

void List<void*>::clear()
{
  node_t *cur = m_First;
  for (size_t i = 0;i < m_Count;i++)
  {
    node_t *tmp = cur;
    cur = cur->next;
    delete tmp;
  }

  m_Count = 0;
  m_First = 0;
  m_Last = 0;
}
void List<void*>::assign(const List<void*> &x)
{
  if (m_Count != 0)clear();

  ConstIterator Cur(x.begin());
  ConstIterator End(x.end());
  for (;Cur != End;++Cur)
    pushBack(*Cur);
}

//
// Explicitly instantiate List<void*>
//
template class List<void*>;
