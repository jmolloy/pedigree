/*
 * Copyright (c) 2010 Eduard Burtescu
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

#ifndef HIDREPORT_H
#define HIDREPORT_H

#include <hid/HidUtils.h>
#include <processor/types.h>
#include <utilities/Vector.h>

class HidReport
{
    public:
        HidReport();
        virtual ~HidReport();

        /// Parses a HID report descriptor and stores the resulted data
        void parseDescriptor(uint8_t *pDescriptor, size_t nDescriptorLength);

        /// Feeds the input interpreter with a new input buffer
        void feedInput(uint8_t *pBuffer, uint8_t *pOldBuffer, size_t nBufferSize);

    private:
        /// The HID item types
        enum ItemType
        {
            MainItem    = 0,
            GlobalItem  = 1,
            LocalItem   = 2
        };

        /// The HID item tags
        enum ItemTag
        {
            // Main items tags
            InputItem       = 0x8,
            OutputItem      = 0x9,
            FeatureItem     = 0xb,
            CollectionItem  = 0xa,
            EndCollectionItem   = 0xc,

            // Global items tags
            UsagePageItem   = 0x0,
            LogMinItem      = 0x1,
            LogMaxItem      = 0x2,
            PhysMinItem     = 0x3,
            PhysMaxItem     = 0x4,
            UnitExpItem     = 0x5,
            UnitItem        = 0x6,
            ReportSizeItem  = 0x7,
            ReportIDItem    = 0x8,
            ReportCountItem = 0x9,
            PushItem        = 0xa,
            PopItem         = 0xb,

            // Local items tags
            UsageItem       = 0x0,
            UsageMinItem    = 0x1,
            UsageMaxItem    = 0x2
        };

        /// Input item flags
        enum InputItemFlags
        {
            InputConstant = 1,
            InputVariable = 2,
            InputRelative = 4
        };

        /// Structure holding various global and local values for a Main item
        struct LocalState
        {
            /// Constructor, sets all values to ~0 (invalid)
            inline LocalState() :
                nUsagePage(~0), nLogMin(~0), nLogMax(~0), nPhysMin(~0), nPhysMax(~0),
                nReportSize(~0), nReportID(~0), nReportCount(~0),
                pUsages(0), nUsageMin(~0), nUsageMax(~0) {}

            inline virtual ~LocalState()
            {
                // The parser is required to set pUsages to zero when it's actually used
                if(pUsages)
                    delete pUsages;
            }

            // Global values
            int64_t nUsagePage;
            int64_t nLogMin;
            int64_t nLogMax;
            int64_t nPhysMin;
            int64_t nPhysMax;
            int64_t nReportSize;
            int64_t nReportID;
            int64_t nReportCount;

            // Local values
            Vector<size_t> *pUsages;
            int64_t nUsageMin;
            int64_t nUsageMax;

            /// Resets the local values (called every time a Main item occurs)
            void resetLocalValues()
            {
                // The parser is required to set pUsages to zero when it's actually used
                if(pUsages)
                {
                    delete pUsages;
                    pUsages = 0;
                }
                nUsageMin = ~0;
                nUsageMax = ~0;
            }

            /// Returns the usage number represented by the given index
            uint16_t getUsageByIndex(uint16_t nUsageIndex)
            {
                // If there's an usage vector, return the usage value in it or zero if the index is not valid
                if(pUsages)
                    return nUsageIndex < pUsages->count() ? (*pUsages)[nUsageIndex] : 0;

                // The usage value if in range, 0 otherwise
                return (nUsageMin + nUsageIndex) <= nUsageMax ? nUsageMin + nUsageIndex : 0;
            }

            /// Copy constructor
            LocalState &operator = (LocalState &s)
            {
                // Copy all the data we need
                nUsagePage = s.nUsagePage;
                nLogMin = s.nLogMin;
                nLogMax = s.nLogMax;
                nPhysMin = s.nPhysMin;
                nPhysMax = s.nPhysMax;
                nReportSize = s.nReportSize;
                nReportID = s.nReportID;
                nReportCount = s.nReportCount;
                pUsages = s.pUsages;
                nUsageMin = s.nUsageMin;
                nUsageMax = s.nUsageMax;

                // Make sure the usage vector won't get deleted
                s.pUsages = 0;

                return *const_cast<LocalState*>(this);
            }
        };

        /// Structure representing an Input block whitin a Collection
        struct InputBlock
        {
            /// Feeds input to this block
            void feedInput(uint8_t *pBuffer, uint8_t *pOldBuffer, size_t nBufferSize, size_t &nBitOffset, HidDeviceType deviceType);

            /// The type of the input block
            enum
            {
                Constant,
                Absolute,
                Relative,
                Array
            } type;

            /// The local state of the input block
            LocalState state;
        };

        /// Structure representing a Collection whitin a report
        struct Collection
        {
            // Trickery to allow putting different type childs in the same vector
            enum ChildType
            {
                CollectionChild,
                InputBlockChild
            };
            struct Child
            {
                inline Child(Collection *pC) : type(CollectionChild), pCollection(pC) {}
                inline Child(InputBlock *pIB) : type(InputBlockChild), pInputBlock(pIB) {}

                ChildType type;
                union
                {
                    Collection *pCollection;
                    InputBlock *pInputBlock;
                };
            };

            /// Feeds input to this collection (will get forwarded to the lowest collection)
            void feedInput(uint8_t *pBuffer, uint8_t *pOldBuffer, size_t nBufferSize, size_t &nBitOffset);

            /// Guesses from which type of device the input associated with this collection comes from
            HidDeviceType guessInputDevice();

            /// All the childs in this collection
            Vector<Child*> childs;

            /// Our parent
            Collection *pParent;

            /// The local state of the collection
            LocalState state;
        };

        /// The root collection, under which everything is
        Collection *m_pRootCollection;
};

#endif
