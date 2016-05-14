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

#ifndef KERNEL_UTILITIES_CPP_H
#define KERNEL_UTILITIES_CPP_H

#ifdef __cplusplus

#include <processor/types.h>
#include <utilities/lib.h>

namespace pedigree_std
{
template <typename T1, typename T2>
struct is
{
    static const bool value = false;
};

template <typename T1, typename T2>
struct is_not
{
    static const bool value = true;
};

template <typename T>
struct is<T, T>
{
    static const bool value = true;
};

template <typename T>
struct is_not<T, T>
{
    static const bool value = false;
};

template<typename T>
struct is_integral
{
    static const bool value = false;
};
template<>struct is_integral<bool>{static const bool value = true;};
template<>struct is_integral<char>{static const bool value = true;};
template<>struct is_integral<unsigned char>{static const bool value = true;};
template<>struct is_integral<signed char>{static const bool value = true;};
template<>struct is_integral<short>{static const bool value = true;};
template<>struct is_integral<unsigned short>{static const bool value = true;};
template<>struct is_integral<int>{static const bool value = true;};
template<>struct is_integral<unsigned int>{static const bool value = true;};
template<>struct is_integral<long>{static const bool value = true;};
template<>struct is_integral<unsigned long>{static const bool value = true;};
template<>struct is_integral<long long>{static const bool value = true;};
template<>struct is_integral<unsigned long long>{static const bool value = true;};

template<typename T>
struct is_pointer
{
    static const bool value = false;
};
template<typename T>
struct is_pointer<T*>
{
    static const bool value = true;
};

/** remove_all_extents gets a bare (e.g. T) type from array types (e.g. T[]). */
template<class T>
struct remove_all_extents
{
    typedef T type;
};

template<class T>
struct remove_all_extents<T[]>
{
    typedef typename remove_all_extents<T>::type type;
};

template<class T, size_t N>
struct remove_all_extents<T[N]>
{
    typedef typename remove_all_extents<T>::type type;
};

/** Enable if the given expression is true. */
template<bool, class T = void>
struct enable_if
{
};

template<class T>
struct enable_if<true, T>
{
    typedef T type;
};

/** Provide a static integral constant. */
template<class T, T v>
struct integral_constant
{
    static constexpr T value = v;
    typedef T value_type;
    typedef integral_constant type;
    constexpr operator value_type() const noexcept
    {
       return value;
    }
};

template<class T, T v>
constexpr T integral_constant<T, v>::value;

/** is_scalar checks for scalar types */
/// \todo this should check a little more than just this
template<class T>
struct is_scalar : integral_constant<bool,
    is_integral<T>::value || is_pointer<T>::value>
{
};

/** Identify a type as trivial (e.g. default constructor, scalar type). */
template<class T>
struct is_trivial : public integral_constant<bool, __is_trivial(T)>
{
};

/** Identify a type as trivially copyable (can memcpy it). */
template<class T>
struct is_trivially_copyable :
    public integral_constant<bool,
        is_scalar<typename remove_all_extents<T>::type>::value>
{
};

/** Perform a copy in the easiest way possible. */
template<class T>
typename enable_if<is_trivially_copyable<T>::value>::type *
copy(T *dest, const T *src, size_t count)
{
    return MemoryCopy(dest, src, count * sizeof(T));
}

/** Perform a copy in the easiest way possible. */
template<class T>
typename enable_if<!is_trivially_copyable<T>::value>::type *
copy(T *dest, const T *src, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        dest[i] = src[i];
    }
    return dest;
}

/** Obtain information about a function. */
template<class T>
struct function_traits : public function_traits<decltype(&T::operator())>
{
};

template <class C, class R, class... Args>
struct function_traits<R (C::*) (Args...) const>
{
    typedef R return_type;
    typedef C class_type;
};

template <class R, class... Args>
struct function_traits<R (*)(Args...)>
{
    typedef R return_type;
};

template <class R, class... Args>
struct function_traits<R(Args...)>
{
    typedef R return_type;
};

/** Wraps a callable (e.g. lambda) such that it can be called later. */
template <class F>
class Callable
{
    public:
        Callable(const F &x) : func(x)
        {
        }

        template <class... Args>
        typename function_traits<F>::return_type
        operator () (Args... args)
        {
            return func(args...);
        }

    private:
        const F &func;
};

/** Creates a callable. */
template <class T>
Callable<T> make_callable(T &f)
{
    return Callable<T>(f);
}

/** Remove pointer type from the given type. */
template <class T>
struct remove_pointer
{
    typedef T type;
};

template <class T>
struct remove_pointer<T *>
{
    typedef T type;
};

}  // namespace pedigree_std

using pedigree_std::is_integral;
using pedigree_std::is_pointer;

#endif  // __cplusplus

#endif  // KERNEL_UTILITIES_CPP_H
