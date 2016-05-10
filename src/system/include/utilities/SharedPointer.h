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

#ifndef SHAREDPOINTER_H
#define SHAREDPOINTER_H

#include <processor/types.h>

/**
 * Provides a reference-counted pointer that can be freely shared, and is
 * automatically destroyed when the last owner's instance is destructed.
 */
template <class T>
class SharedPointer
{
public:
    /**
     * Default instantiation, without an associated pointer object.
     */
    SharedPointer();

    /**
     * Instantiate, owning the given memory region.
     */
    SharedPointer(T *ptr);

    /**
     * Destruction, which automatically frees the pointer if no owners remain.
     */
    virtual ~SharedPointer();

    /**
     * Create another reference to the given SharedPointer.
     */
    SharedPointer(const SharedPointer<T> &p);

    /**
     * Reset this SharedPointer, reducing the refcount on any held object.
     */
    void reset();
    /**
     * Reset this SharedPointer, reducing the refcount on any held object, and
     * then set this SharedPointer to track the given memory.
     */
    void reset(T *ptr);

    /**
     * Retrieve the internal pointer, or null if no object is held.
     */
    T *get() const;

    /**
     * Dereference the internal pointer (null if no object is held).
     */
    T *operator ->() const;

    /**
     * Dereference the internal pointer (dangerous if no object held).
     */
    T &operator *() const;

    /**
     * Release any currently held object, and then reference that held by the
     * given SharedPointer.
     */
    SharedPointer<T> &operator =(const SharedPointer<T> &p);

    /**
     * Whether or not this pointer is valid.
     */
    explicit operator bool() const;

    /**
     * Indicates whether this SharedPointer is only held by one owner.
     */
    bool unique() const;

    /**
     * Indicates this SharedPointer's current reference count.
     */
    size_t refcount() const;

    /**
     * Creates a SharedPointer with a default allocation.
     */
    template<class... Args>
    static SharedPointer<T> allocate(Args...);

    /// \note No operator is provided for comparison with raw pointer types:
    ///       if that comparison were to ever succeed, it would indicate that
    ///       the SharedPointer could not safely free memory after the refcount
    ///       falls to zero.
    bool operator == (const SharedPointer &p) const;
    bool operator != (const SharedPointer &p) const;
    bool operator < (const SharedPointer &p) const;
    bool operator <= (const SharedPointer &p) const;
    bool operator > (const SharedPointer &p) const;
    bool operator >= (const SharedPointer &p) const;

private:
    /**
     * Internal do-er to release a reference to the held object.
     */
    void release();

    /**
     * Main control structure, shared across all SharedPointers referencing
     * the same object and copied from each other.
     */
    struct Control
    {
        T *ptr;
        size_t refcount;
    } *m_Control;
};

template <class T>
SharedPointer<T>::SharedPointer() : m_Control(0)
{
}

template <class T>
SharedPointer<T>::SharedPointer(T *ptr) : m_Control(0)
{
    reset(ptr);
}

template <class T>
SharedPointer<T>::~SharedPointer()
{
    release();
}

template <class T>
SharedPointer<T>::SharedPointer(const SharedPointer<T> &p) : m_Control(0)
{
    release();

    m_Control = p.m_Control;
    if (m_Control)
    {
        __atomic_add_fetch(&m_Control->refcount, 1, __ATOMIC_RELAXED);
    }
}

template <class T>
void SharedPointer<T>::reset()
{
    release();
}

template <class T>
void SharedPointer<T>::reset(T *ptr)
{
    release();

    m_Control = new Control;
    m_Control->refcount = 1;
    m_Control->ptr = ptr;
}

template <class T>
T *SharedPointer<T>::get() const
{
    if (!m_Control)
        return 0;

    return m_Control->ptr;
}

template <class T>
T *SharedPointer<T>::operator ->() const
{
    return get();
}

template <class T>
T &SharedPointer<T>::operator *() const
{
    return *(get());
}

template <class T>
SharedPointer<T> &SharedPointer<T>::operator =(const SharedPointer<T> &p)
{
    release();

    m_Control = p.m_Control;
    if (m_Control)
    {
        __atomic_add_fetch(&m_Control->refcount, 1, __ATOMIC_RELAXED);
    }

    return *this;
}

template <class T>
SharedPointer<T>::operator bool() const
{
    return get() != 0;
}

template <class T>
bool SharedPointer<T>::unique() const
{
    return refcount() == 1;
}

template <class T>
size_t SharedPointer<T>::refcount() const
{
    if (!m_Control)
        return 0;

    return __atomic_load_n(&m_Control->refcount, __ATOMIC_RELAXED);
}

template <class T>
template <class... Args>
SharedPointer<T> SharedPointer<T>::allocate(Args... args)
{
    SharedPointer<T> result;
    result.reset(new T(args...));
    return result;
}

template <class T>
void SharedPointer<T>::release()
{
    if (!m_Control)
        return;

    size_t rc = __atomic_sub_fetch(&m_Control->refcount, 1, __ATOMIC_RELAXED);
    if (!rc)
    {
        /// \todo allow specifying a custom function to handle deletion
        delete m_Control->ptr;
        m_Control->ptr = 0;

        delete m_Control;
    }

    // We don't care about the control structure anymore. Someone else will
    // free it if we did not.
    m_Control = 0;
}

template <class T>
bool SharedPointer<T>::operator == (const SharedPointer &p) const
{
    return get() == p.get();
}

template <class T>
bool SharedPointer<T>::operator != (const SharedPointer &p) const
{
    return get() != p.get();
}

template <class T>
bool SharedPointer<T>::operator < (const SharedPointer &p) const
{
    return get() < p.get();
}

template <class T>
bool SharedPointer<T>::operator <= (const SharedPointer &p) const
{
    return get() <= p.get();
}

template <class T>
bool SharedPointer<T>::operator > (const SharedPointer &p) const
{
    return get() > p.get();
}

template <class T>
bool SharedPointer<T>::operator >= (const SharedPointer &p) const
{
    return get() >= p.get();
}

#endif
