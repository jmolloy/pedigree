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
#ifndef KERNEL_UTILITIES_TEMPLATE_H
#define KERNEL_UTILITIES_TEMPLATE_H

/** @addtogroup kernelutilities
 * @{ */

/** Global != operator is provided for every type that provides a == operator
 *\brief Global != operator for types with overloaded == operator
 *\param[in] x1 first operand
 *\param[in] x2 second operand
 *\return true, if the objects are not equal, false otherwise */
template<class T1, class T2>
bool operator != (const T1 &x1, const T2 &x2)
{
  return !(x1 == x2);
}
/** Global postincrement operator is provided for every type that provides a
 *  preincrement operator.
 *\brief Global postincrement operator for types with overloaded preincrement operator
 *\param[in] x object
 *\return original object */
template<class T>
T operator ++ (T &x, int)
{
  T tmp(x);
  ++x;
  return tmp;
}
/** Global postdecrement operator is provided for every type that provides a
 *  predecrement operator.
 *\brief Global postdecrement operator for types with overloaded predecrement operator
 *\param[in] x object
 *\return original object */
template<class T>
T operator -- (T &x, int)
{
  T tmp(x);
  --x;
  return tmp;
}

/** Remove the const qualifier of a type without const qualifier
 *\brief Remove the const qualifier of a type */
template<typename T>
struct nonconst_type
{
  /** The same type */
  typedef T type;
};
/** Remove the const qualifier of a type with const qualifier
 *\brief Remove the const qualifier of a type */
template<typename T>
struct nonconst_type<const T>
{
  /** The Type without the const qualifier */
  typedef T type;
};

/** @} */

#endif
