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
 */

#include "VFS.h"
#include <Log.h>
#include <Module.h>
#include <utilities/utility.h>

VFS VFS::m_Instance;

VFS &VFS::instance()
{
    return m_Instance;
}

VFS::VFS() :
    m_Aliases(), m_ProbeCallbacks(), m_MountCallbacks()
{
}

VFS::~VFS()
{
}

bool VFS::mount(Disk *pDisk, String &alias)
{
    for (List<Filesystem::ProbeCallback*>::Iterator it = m_ProbeCallbacks.begin();
         it != m_ProbeCallbacks.end();
         it++)
    {
        Filesystem::ProbeCallback cb = **it;
        Filesystem *pFs = cb(pDisk);
        if (pFs)
        {
            if (strlen(alias) == 0)
            {
                alias = pFs->getVolumeLabel();
            }
            alias = getUniqueAlias(alias);
            addAlias(pFs, alias);
            
            if(m_Mounts.lookup(pFs) == 0)
                m_Mounts.insert(pFs, new List<String*>);
            
            m_Mounts.lookup(pFs)->pushBack(new String(alias));

            for (List<MountCallback*>::Iterator it = m_MountCallbacks.begin();
                 it != m_MountCallbacks.end();
                 it++)
            {
                MountCallback mc = *(*it);
                mc();
            }

            return true;
        }
    }
    return false;
}

void VFS::addAlias(Filesystem *pFs, String alias)
{
    if(!pFs)
        return;

    pFs->m_nAliases++;
    m_Aliases.insert(alias, pFs);
    
    if(m_Mounts.lookup(pFs) == 0)
        m_Mounts.insert(pFs, new List<String*>);
    
    m_Mounts.lookup(pFs)->pushBack(new String(alias));
}

void VFS::addAlias(String oldAlias, String newAlias)
{
    Filesystem *pFs = m_Aliases.lookup(oldAlias);
    if(pFs)
    {
        m_Aliases.insert(newAlias, pFs);
        
        if(m_Mounts.lookup(pFs) == 0)
            m_Mounts.insert(pFs, new List<String*>);
        
        m_Mounts.lookup(pFs)->pushBack(new String(newAlias));
    }
}

String VFS::getUniqueAlias(String alias)
{
    if(!aliasExists(alias))
        return alias;

    // <alias>-n is how we keep them unique
    // negative numbers already have a dash
    int32_t index = -1;
    while(true)
    {
        NormalStaticString tmpAlias;
        tmpAlias += static_cast<const char*>(alias);
        tmpAlias.append(index);

        String s = String(static_cast<const char*>(tmpAlias));
        if(!aliasExists(s))
            return s;
        index--;
    }

    return String();
}


bool VFS::aliasExists(String alias)
{
    return (m_Aliases.lookup(alias) != 0);
}

void VFS::removeAlias(String alias)
{
    /// \todo Remove from m_Mounts
    m_Aliases.remove(alias);
}

void VFS::removeAllAliases(Filesystem *pFs)
{
    if(!pFs)
        return;

    for (RadixTree<Filesystem*>::Iterator it = m_Aliases.begin();
         it != m_Aliases.end();
         it++)
    {
        if (pFs == (*it))
        {
            it = m_Aliases.erase(it);
        }
    }
    
    /// \todo Locking.
    if(m_Mounts.lookup(pFs) != 0)
    {
        List<String*>* pList = m_Mounts.lookup(pFs);
        for(List<String*>::Iterator it = pList->begin();
            it != pList->end();
            it++)
        {
            delete *it;
        }
        
        delete pList;
        
        m_Mounts.remove(pFs);
    }
    
    delete pFs;
}

Filesystem *VFS::lookupFilesystem(String alias)
{
    return m_Aliases.lookup(alias);
}

File *VFS::find(String path, File *pStartNode)
{
    // Search for a colon.
    bool bColon = false;
    size_t i;
    for (i = 0; i < path.length(); i++)
    {
        // Look for the UTF-8 '»'; 0xC2 0xBB.
        if (path[i] == '\xc2' && path[i+1] == '\xbb')
        {
            bColon = true;
            break;
        }
    }

    if (!bColon)
    {
        // Pass directly through to the filesystem, if one specified.
        if (!pStartNode) return 0;
        else return pStartNode->getFilesystem()->find(path, pStartNode);
    }
    else
    {
        String newPath = path.split(i+2);
        path.chomp();
        path.chomp();

        // Attempt to find a filesystem alias.
        Filesystem *pFs = lookupFilesystem(path);
        if (!pFs)
            return 0;
        
        return pFs->find(newPath, 0);
    }
}

void VFS::addProbeCallback(Filesystem::ProbeCallback callback)
{
    Filesystem::ProbeCallback *p = new Filesystem::ProbeCallback;
    *p = callback;
    m_ProbeCallbacks.pushBack(p);
}

void VFS::addMountCallback(MountCallback callback)
{
    MountCallback *p = new MountCallback;
    *p = callback;
    m_MountCallbacks.pushBack(p);
}


bool VFS::createFile(String path, uint32_t mask, File *pStartNode)
{
    // Search for a colon.
    bool bColon = false;
    size_t i;
    for (i = 0; i < path.length(); i++)
    {
        // Look for the UTF-8 '»'; 0xC2 0xBB.
        if (path[i] == '\xc2' && path[i+1] == '\xbb')
        {
            bColon = true;
            break;
        }
    }

    if (!bColon)
    {
        // Pass directly through to the filesystem, if one specified.
        if (!pStartNode) return false;
        else return pStartNode->getFilesystem()->createFile(path, mask, pStartNode);
    }
    else
    {
        String newPath = path.split(i+2);
        path.chomp();
        path.chomp();

        // Attempt to find a filesystem alias.
        Filesystem *pFs = lookupFilesystem(path);
        if (!pFs)
            return false;
        return pFs->createFile(newPath, mask, 0);
    }
}

bool VFS::createDirectory(String path, File *pStartNode)
{
    // Search for a colon.
    bool bColon = false;
    size_t i;
    for (i = 0; i < path.length(); i++)
    {
        // Look for the UTF-8 '»'; 0xC2 0xBB.
        if (path[i] == '\xc2' && path[i+1] == '\xbb')
        {
            bColon = true;
            break;
        }
    }

    if (!bColon)
    {
        // Pass directly through to the filesystem, if one specified.
        if (!pStartNode) return false;
        else return pStartNode->getFilesystem()->createDirectory(path, pStartNode);
    }
    else
    {
        // i+2 as the delimiter character (») is two bytes long.
        String newPath = path.split(i+2);
        path.chomp();
        path.chomp();

        // Attempt to find a filesystem alias.
        Filesystem *pFs = lookupFilesystem(path);
        if (!pFs)
            return false;
        return pFs->createDirectory(newPath, 0);
    }
}

bool VFS::createSymlink(String path, String value, File *pStartNode)
{
    // Search for a colon.
    bool bColon = false;
    size_t i;
    for (i = 0; i < path.length(); i++)
    {
        // Look for the UTF-8 '»'; 0xC2 0xBB.
        if (path[i] == '\xc2' && path[i+1] == '\xbb')
        {
            bColon = true;
            break;
        }
    }

    if (!bColon)
    {
        // Pass directly through to the filesystem, if one specified.
        if (!pStartNode) return false;
        else return pStartNode->getFilesystem()->createSymlink(path, value, pStartNode);
    }
    else
    {
        String newPath = path.split(i+2);
        path.chomp();
        path.chomp();

        // Attempt to find a filesystem alias.
        Filesystem *pFs = lookupFilesystem(path);
        if (!pFs)
            return false;
        return pFs->createSymlink(newPath, value, 0);
    }
}

bool VFS::remove(String path, File *pStartNode)
{
    // Search for a colon.
    bool bColon = false;
    size_t i;
    for (i = 0; i < path.length(); i++)
    {
        // Look for the UTF-8 '»'; 0xC2 0xBB.
        if (path[i] == '\xc2' && path[i+1] == '\xbb')
        {
            bColon = true;
            break;
        }
    }

    if (!bColon)
    {
        // Pass directly through to the filesystem, if one specified.
        if (!pStartNode) return false;
        else return pStartNode->getFilesystem()->remove(path, pStartNode);
    }
    else
    {
        String newPath = path.split(i+2);
        path.chomp();
        path.chomp();

        // Attempt to find a filesystem alias.
        Filesystem *pFs = lookupFilesystem(path);
        if (!pFs)
            return false;
        return pFs->remove(newPath, pStartNode);
    }
}

static void initVFS()
{
}

static void destroyVFS()
{
}

MODULE_INFO("vfs", &initVFS, &destroyVFS);
