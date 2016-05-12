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

#ifndef POSIX_PROCESS_H
#define POSIX_PROCESS_H

#include <processor/types.h>
#include <PosixSubsystem.h>
#include <Log.h>

#include <process/Process.h>

class PosixProcess;

class PosixSession
{
    public:
        PosixSession() : Leader(0)
        {
        }

        virtual ~PosixSession()
        {
        }

        /** Session leader. */
        PosixProcess *Leader;
};

class ProcessGroup
{
    public:
        ProcessGroup() : processGroupId(0), Leader(0), Members()
        {
            Members.clear();
        }

        virtual ~ProcessGroup();

        /** The process group ID of this process group. */
        int processGroupId;

        /** The group leader of the process group. */
        PosixProcess *Leader;

        /** List of each Process that is in this process group.
         *  Includes the Leader, iterate over this in order to
         *  obtain every Process in the process group.
         */
        List<PosixProcess*> Members;

    private:
        ProcessGroup(const ProcessGroup&);
        ProcessGroup &operator = (ProcessGroup &);
};

class PosixProcess : public Process
{
    public:

        /** Defines what status this Process has within its group */
        enum Membership
        {
            /** Group leader. The one who created the group, and whose PID was absorbed
             *  to become the Process Group ID.
             */
            Leader = 0,

            /** Group member. These processes have a unique Process ID. */
            Member,

            /** Not in a group. */
            NoGroup
        };

        PosixProcess() :
            Process(), m_pSession(0), m_pProcessGroup(0), m_GroupMembership(NoGroup)
        {}

        /** Copy constructor. */
        PosixProcess(Process *pParent) :
            Process(pParent), m_pSession(0), m_pProcessGroup(0), m_GroupMembership(NoGroup)
        {
            if(pParent->getType() == Posix)
            {
                PosixProcess *pPosixParent = static_cast<PosixProcess *>(pParent);
                m_pSession = pPosixParent->m_pSession;
                setProcessGroup(pPosixParent->getProcessGroup());
                if(m_pProcessGroup)
                {
                    setGroupMembership(Member);
                }
            }
        }

        virtual ~PosixProcess()
        {
        }

        void setProcessGroup(ProcessGroup *newGroup, bool bRemoveFromGroup = true)
        {
            // Remove ourselves from our existing group.
            if(m_pProcessGroup && bRemoveFromGroup)
            {
                for(List<PosixProcess*>::Iterator it = m_pProcessGroup->Members.begin();
                    it != m_pProcessGroup->Members.end();
                    )
                {
                    if((*it) == this)
                    {
                        it = m_pProcessGroup->Members.erase(it);
                    }
                    else
                        ++it;
                }
            }

            // Now join the real group.
            m_pProcessGroup = newGroup;
            if(m_pProcessGroup)
            {
                m_pProcessGroup->Members.pushBack(this);
                NOTICE(">>>>>> Adding self to the members list, new size = " << m_pProcessGroup->Members.count() << ".");

                ProcessGroupManager::instance().setGroupId(m_pProcessGroup->processGroupId);
            }
        }

        ProcessGroup *getProcessGroup()
        {
            return m_pProcessGroup;
        }

        void setGroupMembership(Membership type)
        {
            m_GroupMembership = type;
        }

        Membership getGroupMembership()
        {
            return m_GroupMembership;
        }

        PosixSession *getSession()
        {
            return m_pSession;
        }

        void setSession(PosixSession *p)
        {
            m_pSession = p;
        }

        virtual ProcessType getType()
        {
            return Posix;
        }

    private:
        PosixProcess(const PosixProcess&);
        PosixProcess& operator=(const PosixProcess&);

        PosixSession *m_pSession;
        ProcessGroup *m_pProcessGroup;
        Membership m_GroupMembership;
};

#endif
