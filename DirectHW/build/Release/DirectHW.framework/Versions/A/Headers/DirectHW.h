/*
 * DirectHW.h - userspace part for DirectHW
 *
 * Copyright © 2008-2010 coresystems GmbH <info@coresystems.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

#ifndef __DIRECTHW_H
#define __DIRECTHW_H

#include <stdint.h>

int iopl(int unused);

unsigned char inb(unsigned short addr);
unsigned short inw(unsigned short addr);
unsigned int inl(unsigned short addr);
#ifdef __EA64__
unsigned long inq(unsigned short addr);
#endif

void outb(unsigned char val, unsigned short addr);
void outw(unsigned short val, unsigned short addr);
void outl(unsigned int val, unsigned short addr);
#ifdef __EA64__
void outq(unsigned long val, unsigned short addr);
#endif

void *map_physical(uint64_t phys_addr, size_t len);
void unmap_physical(void *virt_addr, size_t len);

typedef union {
    struct {
#ifdef __BIG_ENDIAN__
        uint32_t hi;
        uint32_t lo;
#else /* __LITTLE_ENDIAN__ */
        uint32_t lo;
        uint32_t hi;
#endif /* __BIG_ENDIAN__ */
    } io32;

    uint64_t io64;
} msr_t;

msr_t rdmsr(int addr);

int wrmsr(int addr, msr_t msr);
int logical_cpu_select(int cpu);

#ifndef INVALID_MSR_LO
#define INVALID_MSR_LO 0x63744857
#endif /*  INVALID_MSR_LO */

#ifndef INVALID_MSR_HI
#define INVALID_MSR_HI 0x44697265
#endif /*  INVALID_MSR_HI */

#endif /* __DIRECTHW_H */
