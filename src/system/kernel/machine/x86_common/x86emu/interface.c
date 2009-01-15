/* Exclaim x86 real-mode emulator
 * Copyright (C) 2008 Alex Smith
 *
 * Parts borrowed from v86d:
 * Copyright (C) 2007 Michal Januszewski <spock@gentoo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * @brief		x86 real-mode emulator.
 */

#include <arch/bios.h>
#include <arch/cpu.h>
#include <arch/io.h>
#include <arch/x86emu.h>

#include <console/kprintf.h>

#include <lib/string.h>

#include <types/list.h>

#include <assert.h>
#include <errno.h>
#include <fatal.h>

#if CONFIG_BIOS_DEBUG
# define dprintf(fmt...)	kprintf(LOG_DEBUG, fmt)
#else
# define dprintf(fmt...)	
#endif

#define SEGOFF2LIN(seg, off)	((ptr_t)(((seg) << 4) + off))

static ptr_t x86emu_halt_addr;
static X86EMU_intrFuncs x86emu_intr_funcs[256];

/*
 * x86emu I/O helpers.
 */

static uint8_t x86emu_inb(uint16_t port) {
	return in8(port);
}

static void x86emu_outb(uint16_t port, uint8_t data) {
	out8(port, data);
}

static uint16_t x86emu_inw(uint16_t port) {
	return in16(port);
}

static void x86emu_outw(uint16_t port, uint16_t data) {
	out16(port, data);
}

static uint32_t x86emu_inl(uint16_t port) {
	return in32(port);
}

static void x86emu_outl(uint16_t port, uint32_t data) {
	out32(port, data);
}

static X86EMU_pioFuncs x86emu_pio_funcs = {
	.inb  = x86emu_inb,
	.outb = x86emu_outb,
	.inw  = x86emu_inw,
	.outw = x86emu_outw,
	.inl  = x86emu_inl,
	.outl = x86emu_outl,
};

/*
 * Core functions
 */

/** Convert BIOS registers to x86emu registers.
 *
 * Converts a BIOS interface registers struct to the x86emu registers.
 *
 * @param regs		Register structure to convert.
 */
static void x86emu_regs_from_bios(bios_regs_t *regs) {
	M.x86.R_EAX  = regs->ax;
	M.x86.R_EBX  = regs->bx;
	M.x86.R_ECX  = regs->cx;
	M.x86.R_EDX  = regs->dx;
	M.x86.R_EDI  = regs->di;
	M.x86.R_ESI  = regs->si;
	M.x86.R_EBP  = regs->bp;
	M.x86.R_ESP  = regs->sp;
	M.x86.R_EFLG = regs->flags;
	M.x86.R_EIP  = regs->ip;
	M.x86.R_CS   = regs->cs;
	M.x86.R_DS   = regs->ds;
	M.x86.R_ES   = regs->es;
	M.x86.R_FS   = regs->fs;
	M.x86.R_GS   = regs->gs;
	M.x86.R_SS   = regs->ss;
}

/** Convert x86emu registers to BIOS registers.
 *
 * Converts the current x86emu registers to a BIOS interface registers struct,
 *
 * @param regs		Register structure to convert to.
 */
static void x86emu_regs_to_bios(bios_regs_t *regs) {
	regs->ax    = M.x86.R_EAX;
	regs->bx    = M.x86.R_EBX;
	regs->cx    = M.x86.R_ECX;
	regs->dx    = M.x86.R_EDX;
	regs->di    = M.x86.R_EDI;
	regs->si    = M.x86.R_ESI;
	regs->bp    = M.x86.R_EBP;
	regs->sp    = M.x86.R_ESP;
	regs->flags = M.x86.R_EFLG;
	regs->ip    = M.x86.R_EIP;
	regs->cs    = M.x86.R_CS;
	regs->ds    = M.x86.R_DS;
	regs->es    = M.x86.R_ES;
	regs->fs    = M.x86.R_FS;
	regs->gs    = M.x86.R_GS;
	regs->ss    = M.x86.R_SS;
}

/** Push a 16-bit value to the x86emu stack.
 *
 * Pushes a 16-bit value onto the x86emu stack.
 *
 * @param value		Value to push.
 */
static void x86emu_push16(uint16_t value) {
	M.x86.R_ESP = (M.x86.R_SP - 2) & 0xFFFF;
	*(uint16_t *)SEGOFF2LIN(M.x86.R_SS, M.x86.R_SP) = value;
}

/** Call an interrupt.
 *
 * Calls an x86emu interrupt.
 *
 * @param num		Interrupt number.
 */
static void x86emu_call_int(int num) {
	uint32_t eflags = M.x86.R_EFLG;

	/* Push return address and flags. */
	x86emu_push16(eflags);
	x86emu_push16(M.x86.R_CS);
	x86emu_push16(M.x86.R_IP);

	M.x86.R_IP   = *(uint16_t *)((ptr_t)(num * 4));
	M.x86.R_CS   = *(uint16_t *)((ptr_t)((num * 4) + 2));
	M.x86.R_EFLG = M.x86.R_EFLG & ~(0x80100);
}

/** Initialize the x86emu BIOS interface.
 *
 * Sets up the x86emu BIOS interface.
 */
static void x86emu_init(void) {
	int i;

	/* Initialize the interrupt and I/O functions. */
	for (i = 0; i < 256; i++) {
		x86emu_intr_funcs[i] = x86emu_call_int;
	}

	X86EMU_setupIntrFuncs(x86emu_intr_funcs);
	X86EMU_setupPioFuncs(&x86emu_pio_funcs);

	/* Write a halt byte that we use to finish execution. */
	x86emu_halt_addr = (ptr_t)bios_mem_alloc(1);
	if(x86emu_halt_addr == 0) {
		fatal("Could not allocate x86emu halt byte");
	}

	*(uint8_t *)x86emu_halt_addr = 0xF4;

	/* Set default FLAGS. IF + IOPL. */
	M.x86.R_EFLG = EFLAGS_IF | 0x3000;
}

/** Entry function for an x86emu task.
 *
 * Entry function for a x86emu BIOS task.
 *
 * @param task		Task to run.
 */
static void x86emu_task_entry(bios_task_t *task) {
	x86emu_regs_from_bios(task->regs);
	M.x86.R_EFLG = EFLAGS_IF | 0x3000;

	x86emu_push16(EFLAGS_IF | 0x3000);
	x86emu_push16(x86emu_halt_addr >> 4);
	x86emu_push16(0x0);

	X86EMU_exec();

	x86emu_regs_to_bios(task->regs);
	bios_task_exit(task);
}

bios_interface_t x86emu_bios_interface = {
	.init = x86emu_init,
	.task_entry = x86emu_task_entry,
};
