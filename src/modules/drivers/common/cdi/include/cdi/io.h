/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#ifndef _CDI_IO_H_
#define _CDI_IO_H_

//#include <stdint.h>

static inline uint16_t cdi_inw(uint16_t _port)
{
	uint16_t result;
	asm volatile ("inw %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline uint8_t cdi_inb(uint16_t _port)
{
	uint8_t result;
	asm volatile ("inb %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline uint32_t cdi_inl(uint16_t _port)
{
	uint32_t result;
	asm volatile("inl %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}



static inline void cdi_outw(uint16_t _port, uint16_t _data)
{
	asm volatile ("outw %0, %1" : : "a" (_data), "Nd" (_port));
}

static inline void cdi_outb(uint16_t _port, uint8_t _data)
{
	asm volatile ("outb %0, %1" : : "a" (_data), "Nd" (_port));
}

static inline void cdi_outl(uint16_t _port, uint32_t _data)
{
	asm volatile ("outl %0, %1" : : "a"(_data), "Nd" (_port));
}

#endif

