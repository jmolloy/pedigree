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

#ifndef _NATIVE_API_BASE_H
#define _NATIVE_API_BASE_H

#include <native-protocol.h>

/// \todo Write a new configuration system API using the new native API scheme.

/**
 * \page module_nativeapi Native API
 * Pedigree offers a native API alongside the more-portable POSIX API. While the
 * POSIX API facilitates writing portable software, it is also a heavy
 * abstraction over the core Pedigree functionality. The native API is able to
 * avoid many of the compromises that this abstraction forces.
 *
 * The native API consists exclusively of C++ objects that can be instantiated
 * by applications. Method calls on these instances automatically handle calling
 * into the kernel if that is necessary.
 *
 * The main set of operations that become simpler when using the native API
 * rather than the POSIX API include, but are not limited to, the following:
 * - Better support for native Pedigree paths (foo»/bar)
 * - More flexible memory mapping provisions
 *
 * \section module_nativeapi_basics Basics
 * Each application class in the native API has a globally unique identifier.
 * When the class is instantiated, the new instance is registered with the
 * kernel using this identifier. The kernel allocates resources for the request
 * and maps the new instance to the kernel instance. This allows for state to
 * be kept both in the kernel and in the application, as needed.
 *
 * Objects are automatically unregistered when they are destroyed. This allows
 * for the use of RAII by applications to ensure resources are cleaned up when
 * the application is finished with them.
 *
 * System calls are made when needed by the application class. These consist of
 * a method ID which is specific to the class, and a parameter block. It is
 * recommended that the parameter block always contain a version field so that
 * past revisions of the parameter block can be handled properly. The kernel
 * class implements a syscall() method that receives the method ID and the
 * parameter block and determines what to do as a result.
 *
 * A @ReturnState object is provided for all native calls which allows for rich
 * metadata about the result of a call to be provided. This is especially useful
 * for allowing the kernel to report exceptions to be raised by the application
 * class. Indeed, all errors should be reported using C++ exceptions rather than
 * integral statuses.
 *
 * \section module_nativeapi_howto Adding to the API
 * The process for adding a new API to the native API is reasonably simple.
 *
 * First, define a global unique identifier in \ref nativeSyscallNumbers.h.
 *
 * Having created this identifier, create the kernel and application classes.
 * The kernel class should inherit from `NativeBase`, and the application class
 * should inherit from `Object`. Override `Object::guid` on the application class.
 *
 * Add the creation of an instance of the kernel class to
 * `NativeSyscallManager::factory`
 *
 * Override `NativeBase::syscall` on the kernel class. It is acceptable to
 * initially write a stub which always returns 'unsuccessful'.
 *
 * Implement functionality in the application and kernel classes as needed.
 *
 * \section module_nativeapi_categories API Categories
 * The following are a list of general API categories in which most APIs fit.
 *
 * ...
 */

/**
 * Base class for all kernel-space native API implementations.
 */
class NativeBase
{
    public:
        NativeBase() {};
        virtual ~NativeBase() {};

        /**
         * System call entry.
         *
         * This is where every NATIVE_CALL syscall ends up for each kernel-space
         * object.
         * \param subid the method call ID specific to this class
         * \param params parameter block for this class
         */
        /// \todo this definition needs work
        virtual ReturnState syscall(uint64_t subid, void *params, size_t params_size) = 0;
};

#endif
