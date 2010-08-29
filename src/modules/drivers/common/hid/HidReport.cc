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

#include <hid/HidReport.h>
#include <hid/HidUsages.h>
#include <hid/HidUtils.h>
#include <utilities/PointerGuard.h>
#include <Log.h>

// Handy macro for mixing tag and type in a single value
#define MIX_TYPE_N_TAG(type, tag) (type | (tag << 2))

// Log macro that also outputs a number of tabs before the text
#define TABBED_LOG(tabs, text) \
    do \
    { \
        char *sTabs = new char[(tabs * 4) + 1]; \
        memset(sTabs, ' ', tabs * 4); \
        sTabs[tabs * 4] = '\0'; \
        DEBUG_LOG(sTabs << text); \
        delete [] sTabs; \
    } \
    while (0)

#define ITEM_LOG(tabs, type, value) TABBED_LOG(tabs, type << " (" << value << ")" << Hex)
#define ITEM_LOG_DEC(tabs, type, value) TABBED_LOG(tabs, Dec << type << " (" << value << ")" << Hex)

HidReport::HidReport()
{
}

HidReport::~HidReport()
{
}

void HidReport::parseDescriptor(uint8_t *pDescriptor, size_t nDescriptorLength)
{
    // This will store all the values that change during the parsing
    LocalState currentState;

    // Whether PhysMin and PhysMax form a pair
    bool bPhysPair = false;
    // Whether LogMin and LogMax form a pair
    bool bLogPair = false;

    // Pointer to the collection under which we are parsing
    Collection *pCurrentCollection = 0;

    // The depth of the Collection tree
    size_t nDepth = 0;

    // Parse every item in the descriptor
    for(size_t i = 0; i < nDescriptorLength; i++)
    {
        // A union and structure used to unpack the item's data
        union
        {
            struct
            {
                uint8_t size : 2;
                uint8_t type : 2;
                uint8_t tag : 4;
            } PACKED;
            uint8_t raw;
        } item;

        item.raw = pDescriptor[i];
        uint8_t size = item.size == 3 ? 4 : item.size;

        // Get the value
        uint32_t value = 0;
        if(size == 1)
            value = pDescriptor[i + 1];
        else if(size == 2)
            value = pDescriptor[i + 1] | (pDescriptor[i + 2] << 8);
        else if(size == 4)
            value = pDescriptor[i + 1] | (pDescriptor[i + 2] << 8) | (pDescriptor[i + 3] << 16) | (pDescriptor[i + 4] << 24);

        // Update the byte counter (we may hit a continue)
        i += size;

        // Don't allow for Main items (Input, Output, Feature, etc.) outside a collection
        if(!pCurrentCollection && item.type == MainItem && item.tag !=  CollectionItem)
            continue;

        // Check for type and tag to find which item do we have
        switch(MIX_TYPE_N_TAG(item.type, item.tag))
        {
            // Main items
            case MIX_TYPE_N_TAG(MainItem, InputItem):
            {
                // Create a new InputBlock and set the state and type
                InputBlock *pBlock = new InputBlock();
                pBlock->state = currentState;
                if(value & InputConstant)
                {
                    pBlock->type = InputBlock::Constant;
                    ITEM_LOG(nDepth, "Input", "Constant");
                }
                else
                {
                    if(value & InputVariable)
                    {
                        if(value & InputRelative)
                        {
                            pBlock->type = InputBlock::Relative;
                            ITEM_LOG(nDepth, "Input", "Data, Variable, Relative");
                        }
                        else
                        {
                            pBlock->type = InputBlock::Absolute;
                            ITEM_LOG(nDepth, "Input", "Data, Variable, Absolute");
                        }
                    }
                    else
                    {
                        pBlock->type = InputBlock::Array;
                        ITEM_LOG(nDepth, "Input", "Data, Array");
                    }
                }

                // Push it into the child vector
                pCurrentCollection->childs.pushBack(new Collection::Child(pBlock));
                break;
            }
            case MIX_TYPE_N_TAG(MainItem, CollectionItem):
            {
                // Create a new Collection and set the state
                Collection *pCollection = new Collection();
                pCollection->pParent = pCurrentCollection;
                pCollection->state = currentState;

                // Push it into the child vector
                if(pCurrentCollection)
                    pCurrentCollection->childs.pushBack(new Collection::Child(pCollection));

                // We now entered the collection
                pCurrentCollection = pCollection;
                ITEM_LOG(nDepth, "Collection",  value);
                nDepth++;
                break;
            }
            case MIX_TYPE_N_TAG(MainItem, EndCollectionItem):
                // Move up to the parent
                if(pCurrentCollection)
                {
                    // This must be the root collection
                    if(!pCurrentCollection->pParent)
                        m_pRootCollection = pCurrentCollection;
                    pCurrentCollection = pCurrentCollection->pParent;
                }

                nDepth--;
                TABBED_LOG(nDepth, "End Collection");
                break;

            // Global items (set various global variables)
            case MIX_TYPE_N_TAG(GlobalItem, UsagePageItem):
                currentState.nUsagePage = value;
                ITEM_LOG_DEC(nDepth, "Usage Page", value);
                break;
            case MIX_TYPE_N_TAG(GlobalItem, LogMinItem):
                currentState.nLogMin = value;

                if(currentState.nLogMax != ~0 && !bLogPair)
                {
                    HidUtils::fixNegativeMinimum(currentState.nLogMin, currentState.nLogMax);
                    bLogPair = true;
                    ITEM_LOG(nDepth, "Logical Minimum", currentState.nLogMin);
                    ITEM_LOG(nDepth, "Logical Maximum", currentState.nLogMax);
                }
                else
                    bLogPair = false;
                break;
            case MIX_TYPE_N_TAG(GlobalItem, LogMaxItem):
                currentState.nLogMax = value;

                if(currentState.nLogMin != ~0 && !bLogPair)
                {
                    HidUtils::fixNegativeMinimum(currentState.nLogMin, currentState.nLogMax);
                    bLogPair = true;
                    ITEM_LOG(nDepth, "Logical Minimum", currentState.nLogMin);
                    ITEM_LOG(nDepth, "Logical Maximum", currentState.nLogMax);
                }
                else
                    bLogPair = false;
                break;
            case MIX_TYPE_N_TAG(GlobalItem, PhysMinItem):
                currentState.nPhysMin = value;

                if(currentState.nPhysMax != ~0 && !bPhysPair)
                {
                    HidUtils::fixNegativeMinimum(currentState.nPhysMin, currentState.nPhysMax);
                    bPhysPair = true;
                    ITEM_LOG_DEC(nDepth, "Physical Minimum", currentState.nPhysMin);
                    ITEM_LOG_DEC(nDepth, "Physical Maximum", currentState.nPhysMax);
                }
                else
                    bPhysPair = false;
                break;
            case MIX_TYPE_N_TAG(GlobalItem, PhysMaxItem):
                currentState.nPhysMax = value;

                if(currentState.nPhysMin != ~0 && !bPhysPair)
                {
                    HidUtils::fixNegativeMinimum(currentState.nPhysMin, currentState.nPhysMax);
                    bPhysPair = true;
                    ITEM_LOG_DEC(nDepth, "Physical Minimum", currentState.nPhysMin);
                    ITEM_LOG_DEC(nDepth, "Physical Maximum", currentState.nPhysMax);
                }
                else
                    bPhysPair = false;
                break;
            case MIX_TYPE_N_TAG(GlobalItem, ReportSizeItem):
                currentState.nReportSize = value;
                ITEM_LOG_DEC(nDepth, "Report Size", value);
                break;
            case MIX_TYPE_N_TAG(GlobalItem, ReportIDItem):
                currentState.nReportID = value;
                ITEM_LOG_DEC(nDepth, "Report ID", value);
                break;
            case MIX_TYPE_N_TAG(GlobalItem, ReportCountItem):
                currentState.nReportCount = value;
                ITEM_LOG_DEC(nDepth, "Report Count", value);
                break;

            // Local items (set various local variables, mostly usage-related)
            case MIX_TYPE_N_TAG(LocalItem, UsageItem):
                if(!currentState.pUsages)
                    currentState.pUsages = new Vector<size_t>();
                currentState.pUsages->pushBack(value);
                ITEM_LOG_DEC(nDepth, "Usage", value);
                break;
            case MIX_TYPE_N_TAG(LocalItem, UsageMinItem):
                currentState.nUsageMin = value;
                ITEM_LOG_DEC(nDepth, "Usage Minimum", value);
                break;
            case MIX_TYPE_N_TAG(LocalItem, UsageMaxItem):
                currentState.nUsageMax = value;
                ITEM_LOG_DEC(nDepth, "Usage Maximum", value);
                break;
            default:
                ITEM_LOG_DEC(nDepth, "Unknown", item.type << " " << item.tag << " " << Hex << value);
        }

        // Reset the local values in currentState (will also delete the usage vector if it's not used)
        if(item.type == MainItem)
            currentState.resetLocalValues();
    }
}

void HidReport::feedInput(uint8_t *pBuffer, uint8_t *pOldBuffer, size_t nBufferSize)
{
    // Do we have the root collection?
    if(!m_pRootCollection)
        return;

    // Send the input to the root collection
    size_t nBitOffset = 0;
    m_pRootCollection->feedInput(pBuffer, pOldBuffer, nBufferSize, nBitOffset);

    // Move the input to the old buffer, now that it's been parsed
    memcpy(pOldBuffer, pBuffer, nBufferSize);
}

void HidReport::Collection::feedInput(uint8_t *pBuffer, uint8_t *pOldBuffer, size_t nBufferSize, size_t &nBitOffset)
{
    // Send input to each child
    for(size_t i = 0; i < childs.count(); i++)
    {
        Child *pChild = childs[i];

        // If it's a collection, just forward the arguments
        if(pChild->type == CollectionChild)
            pChild->pCollection->feedInput(pBuffer, pOldBuffer, nBufferSize, nBitOffset);

        // If it's an input block, we need to send also a guessed device type
        if(pChild->type == InputBlockChild)
            pChild->pInputBlock->feedInput(pBuffer, pOldBuffer, nBufferSize, nBitOffset, guessInputDevice());
    }
}

HidDeviceType HidReport::Collection::guessInputDevice()
{
    // Go all the way up looking for valid usages
    Collection *pCollection = this;
    while(pCollection)
    {
        // Check for Mouse, Joystick, Keyboard and Keypad usages
        if(pCollection->state.nUsagePage == HidUsagePages::GenericDesktop)
        {
            if(pCollection->state.getUsageByIndex(0) == HidUsages::Mouse)
                return Mouse;
            if(pCollection->state.getUsageByIndex(0) == HidUsages::Joystick)
                return Joystick;
            if(pCollection->state.getUsageByIndex(0) == HidUsages::Keyboard)
                return Keyboard;
            if(pCollection->state.getUsageByIndex(0) == HidUsages::Keypad)
                return Keyboard;
        }

        // Go up
        pCollection = pCollection->pParent;
    }

    // We found nothing
    return UnknownDevice;
}

void HidReport::InputBlock::feedInput(uint8_t *pBuffer, uint8_t *pOldBuffer, size_t nBufferSize, size_t &nBitOffset, HidDeviceType deviceType)
{
    // Check for report IDs
    if(state.nReportID != ~0)
    {
        /// \todo Do implement support
        /// \note This is harder that just checking the first byte
        /// \note Proper support for old buffer data is needed, too
        WARNING("HidReport::InputBlock::feedInput: TODO: Implement support for report IDs");
        return;
    }

    // Compute the size of this block
    size_t nBlockSize = state.nReportCount * state.nReportSize;

    // We don't want to cross the end of the buffer and we can skip constant inputs
    if((nBitOffset + nBlockSize > nBufferSize * 8) || type == Constant)
    {
        nBitOffset += nBlockSize;
        return;
    }

    // Process each field
    for(size_t i = 0; i < state.nReportCount; i++)
    {
        uint64_t nValue = HidUtils::getBufferField(pBuffer, nBitOffset + i * state.nReportSize, state.nReportSize);
        int64_t nRelativeValue = 0;
        switch(type)
        {
            case Absolute:
                // Here we have to take the current absolute value and
                // subtract it by the old value to get a relative value
                nRelativeValue = nValue - HidUtils::getBufferField(pOldBuffer, nBitOffset + i * state.nReportSize, state.nReportSize);

                if(nRelativeValue)
                    HidUtils::sendInputToManager(deviceType, state.nUsagePage, state.getUsageByIndex(i), nRelativeValue);
                break;
            case Relative:
                // The actual value is relative
                nRelativeValue = nValue;
                HidUtils::fixNegativeValue(state.nLogMin, state.nLogMax, nRelativeValue);

                if(nRelativeValue)
                    HidUtils::sendInputToManager(deviceType, state.nUsagePage, state.getUsageByIndex(i), nRelativeValue);
                break;
            case Array:
                // A non-zero value in an array means a holded key/button
                if(nValue)
                {
                    // Check if this array entry is new
                    bool bNew = true;
                    for(size_t j = 0; j < state.nReportCount; j++)
                    {
                        if(HidUtils::getBufferField(pOldBuffer, nBitOffset + j * state.nReportSize, state.nReportSize) == nValue)
                        {
                            bNew = false;
                            break;
                        }
                    }

                    // If it's new, we have a keyDown/buttonDown
                    if(bNew)
                        HidUtils::sendInputToManager(deviceType, state.nUsagePage, nValue, 1);
                }
                break;
            // This is to please GCC
            case Constant:
                break;
        }
    }

    // Special case here: check for array entries that disapeared
    // (keys/buttons that were released since last time)
    if(type == Array)
    {
        for(size_t i = 0; i < state.nReportCount; i++)
        {
            uint64_t nOldValue = HidUtils::getBufferField(pOldBuffer, nBitOffset + i * state.nReportSize, state.nReportSize);
            if(nOldValue)
            {
                // Check if this array entry disapeared
                bool bDisapeared = true;
                for(size_t j = 0; j < state.nReportCount; j++)
                {
                    if(HidUtils::getBufferField(pBuffer, nBitOffset + j * state.nReportSize, state.nReportSize) == nOldValue)
                    {
                        bDisapeared = false;
                        break;
                    }
                }

                // If it disapeared, we have a keyUp/buttonUp
                if(bDisapeared)
                    HidUtils::sendInputToManager(deviceType, state.nUsagePage, nOldValue, -1);
            }
        }
    }

    // Update the bit offset
    nBitOffset += nBlockSize;
}
