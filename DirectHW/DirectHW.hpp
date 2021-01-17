#ifndef __DIRECTHW_HPP__
#define __DIRECTHW_HPP__

/* DirectHW - Kernel extension to pass through IO commands to user space
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

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOMemoryDescriptor.h>

#if defined(__i386__) || defined(__x86_64__)
#include <architecture/i386/pio.h>
#endif /* __i386__ || __x86_64__ */

#ifndef DIRECTHW_VERSION
#define DIRECTHW_VERSION "1.4"
#endif /* DIRECTHW_VERSION */

#ifndef DIRECTHW_VERNUM
#define DIRECTHW_VERNUM 0x00100400
#endif /* DIRECTHW_VERNUM */

#ifndef APPLE_KEXT_OVERRIDE
#ifdef __clang__
#define APPLE_KEXT_OVERRIDE override
#else /* !__clang__ */
#define APPLE_KEXT_OVERRIDE
#endif /* __clang__ */
#endif /* APPLE_KEXT_OVERRIDE */
/* */

#ifndef rdmsr
#define rdmsr(msr, lo, hi) \
    __asm__ volatile("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr))
#endif /* rdmsr */

#ifndef wrmsr
#define wrmsr(msr, lo, hi) \
    __asm__ volatile("wrmsr" : : "c" (msr), "a" (lo), "d" (hi))
#endif /*  wrmsr */

class DirectHWService : public IOService
{
	OSDeclareDefaultStructors(DirectHWService)

public:
	virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
};

/* */
/*class DirectHWService;*/

class DirectHWUserClient : public IOUserClient
{
	OSDeclareDefaultStructors(DirectHWUserClient)

	enum {
		kReadIO,
		kWriteIO,
		kPrepareMap,
		kReadMSR,
		kWriteMSR,
		kNumberOfMethods
	};

	typedef struct {
#if defined(__x86_64__) || defined(__arm64__)
        UInt64 offset;
        UInt64 width;
        UInt64 data;
#else /* __i386__ || __arm__ */
		UInt32 offset;
		UInt32 width;
		UInt32 data;
#endif /* __i386__ || __x86_64__ || __arm__ || __arm64__ */
	} iomem_t;

	typedef struct {
#if defined(__x86_64__) || defined(__arm64__)
		UInt64 addr;
		UInt64 size;
#else /* __i386__ || __arm__ */
        UInt32 addr;
        UInt32 size;
#endif /* __i386__ || __x86_64__ || __arm__ || __arm64__ */
	} map_t;

	typedef struct {
        UInt32 core;
        UInt32 index;

        union {
            uint64_t io64;

            struct
            {
#ifndef __BIG_ENDIAN__
                UInt32 lo;
                UInt32 hi;
#else /* __BIG_ENDIAN__ == 1 */
                UInt32 hi;
                UInt32 lo;
#endif /* __BIG_ENDIAN__ */
            } io32;
        } val;
	} msrcmd_t;

public:
	virtual bool initWithTask(task_t task, void *securityID, UInt32 type) APPLE_KEXT_OVERRIDE;

	virtual bool start(IOService * provider) APPLE_KEXT_OVERRIDE;
	virtual void stop(IOService * provider) APPLE_KEXT_OVERRIDE;

	virtual IOReturn clientMemoryForType(UInt32 type, UInt32 *flags, IOMemoryDescriptor **memory) APPLE_KEXT_OVERRIDE;

	virtual IOReturn clientClose(void) APPLE_KEXT_OVERRIDE;

protected:
	DirectHWService *fProvider;

	static const IOExternalMethod fMethods[kNumberOfMethods];
    static const IOExternalAsyncMethod fAsyncMethods[kNumberOfMethods];

	virtual IOExternalMethod *getTargetAndMethodForIndex(LIBKERN_RETURNS_NOT_RETAINED IOService ** target, UInt32 index) APPLE_KEXT_OVERRIDE;
    virtual IOExternalAsyncMethod *getAsyncTargetAndMethodForIndex(LIBKERN_RETURNS_NOT_RETAINED IOService ** target, UInt32 index) APPLE_KEXT_OVERRIDE;

	virtual IOReturn ReadIO(iomem_t *inStruct, iomem_t *outStruct,
                            IOByteCount inStructSize,
                            IOByteCount *outStructSize);

    virtual IOReturn ReadIOAsync(OSAsyncReference asyncRef,
                                 iomem_t *inStruct, iomem_t *outStruct,
                                 IOByteCount inStructSize,
                                 IOByteCount *outStructSize);

	virtual IOReturn WriteIO(iomem_t *inStruct, iomem_t *outStruct,
                             IOByteCount inStructSize,
                             IOByteCount *outStructSize);

    virtual IOReturn WriteIOAsync(OSAsyncReference asyncRef,
                                  iomem_t *inStruct, iomem_t *outStruct,
                                  IOByteCount inStructSize,
                                  IOByteCount *outStructSize);

	virtual IOReturn PrepareMap(map_t *inStruct, map_t *outStruct,
                                IOByteCount inStructSize,
                                IOByteCount *outStructSize);

    virtual IOReturn PrepareMapAsync(OSAsyncReference asyncRef,
                                     map_t *inStruct, map_t *outStruct,
                                     IOByteCount inStructSize,
                                     IOByteCount *outStructSize);

	virtual IOReturn ReadMSR(msrcmd_t *inStruct, msrcmd_t *outStruct,
                             IOByteCount inStructSize,
                             IOByteCount *outStructSize);

    virtual IOReturn ReadMSRAsync(OSAsyncReference asyncRef,
                                  msrcmd_t *inStruct, msrcmd_t *outStruct,
                                  IOByteCount inStructSize,
                                  IOByteCount *outStructSize);
    
	virtual IOReturn WriteMSR(msrcmd_t *inStruct, msrcmd_t *outStruct,
                              IOByteCount inStructSize,
                              IOByteCount *outStructSize);

    virtual IOReturn WriteMSRAsync(OSAsyncReference asyncRef,
                                   msrcmd_t *inStruct, msrcmd_t *outStruct,
                                   IOByteCount inStructSize,
                                   IOByteCount *outStructSize);

private:
	task_t fTask;
    UInt64 LastMapAddr;
    UInt64 LastMapSize;

	static void MSRHelperFunction(void *data);

    typedef struct {
        msrcmd_t *in;
        msrcmd_t *out;
        bool Read;
    } MSRHelper;

	static inline void cpuid(uint32_t op1, uint32_t op2, uint32_t *data);
};

extern "C"
{
    /* from sys/osfmk/i386/mp.c */
    extern void mp_rendezvous(void (*setup_func)(void *),
                              void (*action_func)(void *),
                              void (*teardown_func)(void *),
                              void *arg);

    extern void mp_rendezvous_no_intrs(void (*action_func)(void *),
                                       void *arg);

    extern int cpu_number(void);
}

#ifndef INVALID_MSR_LO
#define INVALID_MSR_LO 0x63744857
#endif /*  INVALID_MSR_LO */

#ifndef INVALID_MSR_HI
#define INVALID_MSR_HI 0x44697265
#endif /*  INVALID_MSR_HI */

#endif /* __DIRECTHW_HPP__ */
