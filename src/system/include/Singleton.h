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
 *
 * Highly inspired by a talk with Peter Bindels (aka. Candy) in #osdev and his singleton code for AtlantisOS
 * ( http://atlantisos.svn.sourceforge.net/viewvc/atlantisos/include/gof/singleton?view=markup ). Glad to
 * see you back osdeving. :-)
 *
 */

#ifndef KERNEL_SINGLETON_H
#define KERNEL_SINGLETON_H

template<class T>
class Singleton
{
  public:
    static T &instance()
    {
      static T m_Instance;
      return m_Instance;
    }

  protected:
    Singleton(){}
    ~Singleton(){}

  private:
    Singleton(const Singleton &);
    Singleton &operator = (const Singleton &);
};

#endif
