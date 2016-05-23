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

#define PEDIGREE_EXTERNAL_SOURCE 1

#include <gtest/gtest.h>

#include <debugger/commands/LocksCommand.h>

#define LOCK_A reinterpret_cast<Spinlock *>(1)
#define LOCK_B reinterpret_cast<Spinlock *>(2)
#define LOCK_C reinterpret_cast<Spinlock *>(3)
#define LOCK_D reinterpret_cast<Spinlock *>(4)

#define CPU_1 1
#define CPU_2 2
#define CPU_3 3
#define CPU_4 4

class PedigreeLocksCommand : public ::testing::Test
{
    public:
        PedigreeLocksCommand() : TestLocksCommand()
        {
            TestLocksCommand.setReady();
            TestLocksCommand.setFatal();
        }

        ~PedigreeLocksCommand()
        {
        }

        LocksCommand TestLocksCommand;
};

TEST_F(PedigreeLocksCommand, EmptyChecksOk)
{
    EXPECT_TRUE(TestLocksCommand.checkState(LOCK_A, CPU_1));
}

TEST_F(PedigreeLocksCommand, GoodOrdering)
{
    // acquire(A)
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_A, CPU_1));

    // release(A) - OK, in-order
    EXPECT_TRUE(TestLocksCommand.lockReleased(LOCK_A, CPU_1));
}

TEST_F(PedigreeLocksCommand, CrossCpuRelease)
{
    // acquire(A) - CPU 1
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_A, CPU_1));

    // release(A) - CPU 2, OK, unlocks CPU 1 and is in-order
    EXPECT_TRUE(TestLocksCommand.lockReleased(LOCK_A, CPU_2));
}

TEST_F(PedigreeLocksCommand, BadOrdering)
{
    // acquire(A)
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_A, CPU_1));

    // acquire(B)
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_B, CPU_1));

    // release(A) - out-of-order!
    ASSERT_DEATH(TestLocksCommand.lockReleased(LOCK_A, CPU_1),
                 "PANIC: Spinlock 1 released out-of-order \\[expected lock 2, state acquired\\].");
}

TEST_F(PedigreeLocksCommand, StateOk)
{
    // acquire(A)
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_A, CPU_1));

    // acquire(B)
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_B, CPU_1));

    // Good state.
    EXPECT_TRUE(TestLocksCommand.checkState(LOCK_B, CPU_1));
}

TEST_F(PedigreeLocksCommand, Inversion)
{
    // acquire(A) - CPU 1
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_A, CPU_1));

    // acquire(B) - CPU 2
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_2));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_B, CPU_2));

    // acquire(B) - CPU 1
    // At this stage, we've set the scene for deadlock, but the deadlock has
    // not yet happened. CPU 2 could release B, still.
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_1));
    EXPECT_TRUE(TestLocksCommand.checkState(LOCK_B, CPU_1));

    // acquire(A) - CPU 2 - fails, due to inversion
    // Because CPU 1 holds A, and CPU 2 holds B, we're in deadlock; neither CPU
    // is able to continue.
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_2));
    ASSERT_DEATH(TestLocksCommand.checkState(LOCK_A, CPU_2),
                 "PANIC: Detected lock dependency inversion \\(deadlock\\) between 1 and 2!");

    // checkState on the other CPU should now break too.
    ASSERT_DEATH(TestLocksCommand.checkState(LOCK_B, CPU_1),
                 "PANIC: Detected lock dependency inversion \\(deadlock\\) between 2 and 1!");
}

TEST_F(PedigreeLocksCommand, Inversion2)
{
    // acquire(A) - CPU 2
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_2));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_A, CPU_2));

    // acquire(B) - CPU 1
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_B, CPU_1));

    // acquire(A) - CPU 1
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_1));
    EXPECT_TRUE(TestLocksCommand.checkState(LOCK_A, CPU_1));

    // acquire(B) - CPU 2 - fails, due to inversion
    // Because CPU 1 holds A, and CPU 2 holds B, we're in deadlock; neither CPU
    // is able to continue.
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_2));
    ASSERT_DEATH(TestLocksCommand.checkState(LOCK_B, CPU_2),
                 "PANIC: Detected lock dependency inversion \\(deadlock\\) between 2 and 1!");

    // checkState on the other CPU should now break too.
    ASSERT_DEATH(TestLocksCommand.checkState(LOCK_A, CPU_1),
                 "PANIC: Detected lock dependency inversion \\(deadlock\\) between 1 and 2!");
}

TEST_F(PedigreeLocksCommand, AlmostInversion)
{
    // acquire(A) - CPU 1
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_1));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_A, CPU_1));

    // acquire(B) - CPU 2
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_2));
    EXPECT_TRUE(TestLocksCommand.lockAcquired(LOCK_B, CPU_2));

    // acquire(B) - CPU 1
    // At this stage, we've set the scene for deadlock, but the deadlock has
    // not yet happened. CPU 2 could release B, still.
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_B, CPU_1));
    EXPECT_TRUE(TestLocksCommand.checkState(LOCK_B, CPU_1));

    // release(B) - CPU 2
    EXPECT_TRUE(TestLocksCommand.lockReleased(LOCK_B, CPU_2));

    // acquire(A) - CPU 2 - OK because B is no longer locked.
    EXPECT_TRUE(TestLocksCommand.lockAttempted(LOCK_A, CPU_2));
    EXPECT_TRUE(TestLocksCommand.checkState(LOCK_A, CPU_2));
}
