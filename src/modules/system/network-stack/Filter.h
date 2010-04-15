/*
 * Copyright (c) 2010 Matthew Iselin, Heights College
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

#include <processor/types.h>
#include <utilities/List.h>
#include <utilities/Tree.h>

/** Provides an interface for filtering network packets as they come in to
  * the system. */
class NetworkFilter
{
    public:
        /// Default constructor, boring.
        NetworkFilter();
        
        /// Destructor, also boring.
        virtual ~NetworkFilter();
        
        /** The design pattern everyone loves to hate! */
        static NetworkFilter & instance()
        {
            return m_Instance;
        }
        
        /** Passes a Level n packet to filter callbacks.
          * Level 1 is the lowest level, handling for example Ethernet frames.
          * Level 2 handles the Level 1 payload. ARP, IP, and other low-level
          *         protocols are handled here.
          * Level 3 handles the Level 2 payload. TCP, UDP, ICMP, etc...
          * Level 4 handles the Level 3 payload. Specific application protocols
          * such as FTP, DNS.
          * \todo Callbacks should be able to return a code which requests a
          *       specific response, such as ICMP Unreachable or something,
          *       rather than just dropping the packet.
          * \param level Level of callback to call
          * \param packet Packet buffer, can be modified by callbacks
          * \param size Size of the packet. Can NOT be modified by callbacks
          * \return False if the packet has been rejected, true otherwise.
          */
        bool filter(size_t level, uintptr_t packet, size_t sz);

        /** Installs a callback for a specific level.
          * \return An identifier which can be passed to removeCallback to
          *         uninstall the callback, or ((size_t) -1) if unable to install.
          */
        size_t installCallback(size_t level, bool (*callback)(uintptr_t, size_t));
        
        /** Removes a callback for a specific level. */
        void removeCallback(size_t level, size_t id);
        
    private:
    
        static NetworkFilter m_Instance;
        
        /// Level -> Callback list mapping
        Tree<size_t, List<void*>* > m_Callbacks;
};

