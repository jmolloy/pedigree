/*
 * 
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

#include <input/Input.h>
#include <pedigree-c/pedigree-syscalls.h>
#include <stdio.h>

using namespace Input;

static void event_callback(size_t p1, size_t p2, uintptr_t* pBuffer, size_t p4)
{
    if(pBuffer[1])
    {
        callback_t cb = reinterpret_cast<callback_t>(pBuffer[1]);
        Input::InputNotification *pNote = reinterpret_cast<Input::InputNotification*>(&pBuffer[2]);
        cb(*pNote);
    }

    pedigree_event_return();
}

void Input::installCallback(CallbackType type, callback_t cb)
{
    pedigree_input_install_callback(reinterpret_cast<void*>(event_callback), static_cast<uint32_t>(type), reinterpret_cast<uintptr_t>(cb));
}

void Input::removeCallback(callback_t cb)
{
    pedigree_input_remove_callback(reinterpret_cast<void*>(cb));
}

void Input::loadKeymapFromFile(const char *path)
{
    // Load the file in to a buffer.
    FILE *pFile = fopen(path, "r");
    if (!pFile)
    {
        fprintf(stderr, "Input::loadKeymapFromFile: Error opening file `%s'.\n", path);
        return;
    }

    // Get the length of the file
    fseek(pFile, 0, SEEK_END);
    size_t nLength = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    // Read the file
    uint8_t *pBuffer = new uint8_t [nLength];
    fread(pBuffer, 1, nLength, pFile);
    fclose(pFile);

    if(pedigree_load_keymap((char*)pBuffer, nLength))
        fprintf(stderr, "Input::loadKeymapFromFile: Error loading keymap\n");
}
