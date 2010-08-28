/*
 * Copyright (c) 2010 Burtescu Eduard
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

#include <machine/HidInputManager.h>
#include <machine/InputManager.h>
#include <machine/KeymapManager.h>
#include <machine/Machine.h>
#include <machine/Timer.h>
#include <LockGuard.h>
#include <Log.h>

HidInputManager HidInputManager::m_Instance;

HidInputManager::HidInputManager()
{
}

HidInputManager::~HidInputManager()
{
}

void HidInputManager::keyDown(uint8_t keyCode)
{
    KeymapManager &keymapManager = KeymapManager::instance();
    
    InputManager::instance().rawKeyUpdate(keyCode, false);

    // Check for modifiers
    if(keymapManager.handleHidModifier(keyCode, true))
    {
        updateKeys();
        return;
    }

    LockGuard<Spinlock> guard(m_KeyLock);

    // Is the key already considered "down"?
    if(!m_KeyStates.lookup(keyCode))
    {
        // If there was no key before, register the timer handler
        if(!m_KeyStates.count())
            Machine::instance().getTimer()->registerHandler(this);

        // Resolve the key
        uint64_t key = keymapManager.resolveHidKeycode(keyCode);

        // Create a key state structure and fill it
        KeyState *keyState = new KeyState;
        keyState->key = key;
        keyState->nLeftTicks = 600000000;

        // Insert the key state
        m_KeyStates.insert(keyCode, keyState);

        // First keypress always sent straight away, repeating keystrokes
        // are transferred as necessary
        if(key)
            InputManager::instance().keyPressed(key);
    }
}

void HidInputManager::keyUp(uint8_t keyCode)
{
    KeymapManager &keymapManager = KeymapManager::instance();
    
    InputManager::instance().rawKeyUpdate(keyCode, true);

    // Check for modifiers
    if(keymapManager.handleHidModifier(keyCode, false))
    {
        updateKeys();
        return;
    }

    LockGuard<Spinlock> guard(m_KeyLock);

    // Is the key actually pressed?
    KeyState *keyState = m_KeyStates.lookup(keyCode);
    if(keyState)
    {
        // Remove the key from the key states tree
        m_KeyStates.remove(keyCode);
        // Delete the key state structure
        delete keyState;
    }
}

void HidInputManager::timer(uint64_t delta, InterruptState &state)
{
    LockGuard<Spinlock> guard(m_KeyLock);

    KeymapManager &keymapManager = KeymapManager::instance();
    for(Tree<uint8_t, KeyState*>::Iterator it = m_KeyStates.begin(); it != m_KeyStates.end(); ++it)
    {
        KeyState *keyState = it.value();
        if(keyState->nLeftTicks > delta)
            keyState->nLeftTicks -= delta;
        else
        {
            keyState->nLeftTicks = 40000000;
            if(keyState->key)
                InputManager::instance().keyPressed(keyState->key);
        }
    }

    // If we've got no more keys being held down, release the handler
    if(!m_KeyStates.count())
        Machine::instance().getTimer()->unregisterHandler(this);
}

void HidInputManager::updateKeys()
{
    LockGuard<Spinlock> guard(m_KeyLock);

    KeymapManager &keymapManager = KeymapManager::instance();
    for(Tree<uint8_t, KeyState*>::Iterator it = m_KeyStates.begin(); it != m_KeyStates.end(); ++it)
    {
        KeyState *keyState = it.value();
        keyState->key = keymapManager.resolveHidKeycode(it.key());
    }
}
