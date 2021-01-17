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

#include "DirectHW.hpp"

#undef DEBUG_KEXT
//#define DEBUG_KEXT

#ifndef suepr
#define super IOService
#endif /* super */

OSDefineMetaClassAndStructors(DirectHWService, IOService)

bool DirectHWService::start(IOService * provider)
{
    IOLog("DirectHW: Driver v%s (compiled on %s) loaded.\nVisit http://www.coresystems.de/ for more information.\n", DIRECTHW_VERSION, __DATE__);

	if (super::start(provider))
    {
		registerService();

		return true;
	}

	return false;
}

#undef  super
#define super IOUserClient

OSDefineMetaClassAndStructors(DirectHWUserClient, IOUserClient)

const IOExternalAsyncMethod DirectHWUserClient::fAsyncMethods[kNumberOfMethods] =
{
    {0, (IOAsyncMethod) & DirectHWUserClient::ReadIOAsync, kIOUCStructIStructO, sizeof(iomem_t), sizeof(iomem_t)},
    {0, (IOAsyncMethod) & DirectHWUserClient::WriteIOAsync, kIOUCStructIStructO, sizeof(iomem_t), sizeof(iomem_t)},
    {0, (IOAsyncMethod) & DirectHWUserClient::PrepareMapAsync, kIOUCStructIStructO, sizeof(map_t), sizeof(map_t)},
    {0, (IOAsyncMethod) & DirectHWUserClient::ReadMSRAsync, kIOUCStructIStructO, sizeof(msrcmd_t), sizeof(msrcmd_t)},
    {0, (IOAsyncMethod) & DirectHWUserClient::WriteMSRAsync, kIOUCStructIStructO, sizeof(msrcmd_t), sizeof(msrcmd_t)}
};

const IOExternalMethod DirectHWUserClient::fMethods[kNumberOfMethods] =
{
	{0, (IOMethod) & DirectHWUserClient::ReadIO, kIOUCStructIStructO, sizeof(iomem_t), sizeof(iomem_t)},
	{0, (IOMethod) & DirectHWUserClient::WriteIO, kIOUCStructIStructO, sizeof(iomem_t), sizeof(iomem_t)},
	{0, (IOMethod) & DirectHWUserClient::PrepareMap, kIOUCStructIStructO, sizeof(map_t), sizeof(map_t)},
	{0, (IOMethod) & DirectHWUserClient::ReadMSR, kIOUCStructIStructO, sizeof(msrcmd_t), sizeof(msrcmd_t)},
	{0, (IOMethod) & DirectHWUserClient::WriteMSR, kIOUCStructIStructO, sizeof(msrcmd_t), sizeof(msrcmd_t)}
};

bool DirectHWUserClient::initWithTask(task_t task, void *securityID, UInt32 type)
{
	bool ret;

	ret = super::initWithTask(task, securityID, type);

#ifdef DEBUG_KEXT
	IOLog("DirectHW: initWithTask(%p, %p, %16lx)\n", (void *)task, (void *)securityID, (unsigned long)type);
#endif /* DEBUG_KEXT */

    if (ret == false)
    {
		IOLog("DirectHW: initWithTask failed.\n");

		return ret;
	}

	fTask = task;

	return ret;
}

IOExternalAsyncMethod *DirectHWUserClient::getAsyncTargetAndMethodForIndex(IOService ** target, UInt32 index)
{
    if (target == NULL)
    {
        return NULL;
    }

    if (index < (UInt32) kNumberOfMethods)
    {
        if (fAsyncMethods[index].object == (IOService *) 0)
        {
            *target = this;
        }

        return (IOExternalAsyncMethod *) & fAsyncMethods[index];
    }

    *target = NULL;
    return NULL;
}

IOExternalMethod *DirectHWUserClient::getTargetAndMethodForIndex(IOService ** target, UInt32 index)
{
    if (target == NULL)
    {
        return NULL;
    }

	if (index < (UInt32) kNumberOfMethods)
    {
		if (fMethods[index].object == (IOService *) 0)
        {
			*target = this;
        }

		return (IOExternalMethod *) & fMethods[index];
	}
    
    *target = NULL;
    return NULL;
}

bool DirectHWUserClient::start(IOService * provider)
{
	bool success;

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Starting DirectHWUserClient\n");
#endif

	fProvider = OSDynamicCast(DirectHWService, provider);
	success = (fProvider != NULL);

	if (kIOReturnSuccess != clientHasPrivilege(current_task(),kIOClientPrivilegeAdministrator)) {
		IOLog("DirectHW: Need to be administrator.\n");
		success = false;
	}

	if (success)
    {
		success = super::start(provider);
#ifdef DEBUG_KEXT
		IOLog("DirectHW: Client successfully started.\n");
	} else {
		IOLog("DirectHW: Could not start client.\n");
#endif
	}
	return success;
}

void DirectHWUserClient::stop(IOService *provider)
{
#ifdef DEBUG_KEXT
	IOLog("DirectHW: Stopping client.\n");
#endif

	super::stop(provider);
}

IOReturn DirectHWUserClient::clientClose(void)
{
	bool success = terminate();
	if (!success) {
		IOLog("DirectHW: Client NOT successfully closed.\n");
#ifdef DEBUG_KEXT
	} else {
		IOLog("DirectHW: Client successfully closed.\n");
#endif
	}

	return kIOReturnSuccess;
}

IOReturn
DirectHWUserClient::ReadIOAsync(OSAsyncReference asyncRef,
                                iomem_t *inStruct, iomem_t *outStruct,
                                IOByteCount inStructSize,
                                IOByteCount *outStructSize)
{
    ((void)asyncRef);

    return DirectHWUserClient::ReadIO(inStruct, outStruct, inStructSize, outStructSize);
}

IOReturn
DirectHWUserClient::ReadIO(iomem_t *inStruct, iomem_t *outStruct,
                           IOByteCount inStructSize,
                           IOByteCount *outStructSize)
{
    ((void)inStructSize);

	if ((fProvider == NULL) || (isInactive()))
    {
		return kIOReturnNotAttached;
	}

	switch (inStruct->width)
    {
	case 1:
		outStruct->data = inb(inStruct->offset);
		break;

	case 2:
		outStruct->data = inw(inStruct->offset);
		break;

	case 4:
		outStruct->data = inl(inStruct->offset);
		break;

#ifdef __LP64__
    case 8:
        outStruct->data = (UInt64)inl(inStruct->offset);
        outStruct->data = ((UInt64)inl(inStruct->offset) << 32);
#endif

	default:
		IOLog("DirectHW: Invalid read attempt %ld bytes at IO address %lx\n",
              (long)inStruct->width, (unsigned long)inStruct->offset);
		break;
	}

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Read %ld bytes at IO address %lx (result=%lx)\n",
          (unsigned long)inStruct->width, (unsigned long)inStruct->offset, (unsigned long)outStruct->data);
#endif /* DEBUG_KEXT */

    if (outStructSize != NULL)
    {
        *outStructSize = sizeof(iomem_t);
    }

	return kIOReturnSuccess;
}

IOReturn
DirectHWUserClient::WriteIOAsync(OSAsyncReference asyncRef, iomem_t *inStruct, iomem_t *outStruct,
                                 IOByteCount inStructSize,
                                 IOByteCount *outStructSize)
{
    ((void)asyncRef);

    return DirectHWUserClient::WriteIO(inStruct, outStruct, inStructSize, outStructSize);
}

IOReturn
DirectHWUserClient::WriteIO(iomem_t *inStruct, iomem_t *outStruct,
                            IOByteCount inStructSize,
                            IOByteCount *outStructSize)
{
#ifndef DEBUG_KEXT
    ((void)inStruct);
#endif /* DEBUG_KEXT */
    ((void)outStruct);
    ((void)inStructSize);

	if ((fProvider == NULL) || (isInactive()))
    {
		return kIOReturnNotAttached;
	} 
	
#ifdef DEBUG_KEXT
	IOLog("DirectHW: Write %ld bytes at IO address %lx (value=%lx)\n",
          (long)inStruct->width, (unsigned long)inStruct->offset, (unsigned long)inStruct->data);
#endif /* DEBUG_KEXT */
	
	switch (inStruct->width)
    {
	case 1:
		outb(inStruct->offset, (unsigned char)inStruct->data);
		break;

	case 2:
		outw(inStruct->offset, (unsigned short)inStruct->data);
		break;

	case 4:
		outl(inStruct->offset, (unsigned int)inStruct->data);
		break;

#ifdef __LP64__
    case 8:
        outl(inStruct->offset, (unsigned int)inStruct->data & 0xFFFFFFFF);
        outl(inStruct->offset, (unsigned int)((inStruct->data & 0xFFFFFFFF00000000) >> 32));
#endif

	default:
		IOLog("DirectHW: Invalid write attempt %ld bytes at IO address %lx\n",
              (long)inStruct->width, (unsigned long)inStruct->offset);
	}

    if (outStructSize != NULL)
    {
        *outStructSize = sizeof(iomem_t);
    }

	return kIOReturnSuccess;
}

IOReturn
DirectHWUserClient::PrepareMapAsync(OSAsyncReference asyncRef,
                                    map_t *inStruct, map_t *outStruct,
                                    IOByteCount inStructSize,
                                    IOByteCount *outStructSize)
{
    ((void)asyncRef);

    return PrepareMap(inStruct, outStruct, inStructSize, outStructSize);
}

IOReturn
DirectHWUserClient::PrepareMap(map_t *inStruct, map_t *outStruct,
                               IOByteCount inStructSize,
                               IOByteCount *outStructSize)
{
    ((void)outStruct);
    ((void)inStructSize);

	if ((fProvider == NULL) || (isInactive()))
    {
		return kIOReturnNotAttached;
	} 

	if ((LastMapAddr != 0) || (LastMapSize != 0))
    {
		return kIOReturnNotOpen;
    }

	LastMapAddr = inStruct->addr;
	LastMapSize = inStruct->size;

#ifdef DEBUG_KEXT
	IOLog("DirectHW: PrepareMap 0x%16lx[0x%lx]\n", (unsigned long)LastMapAddr, (unsigned long)LastMapSize);
#endif /* DEBUG_KEXT */

    if (outStructSize != NULL)
    {
        *outStructSize = sizeof(map_t);
    }

	return kIOReturnSuccess;
}

inline void
DirectHWUserClient::cpuid(uint32_t op1, uint32_t op2, uint32_t *data)
{
	asm("cpuid"
		: "=a" (data[0]),
		  "=b" (data[1]),
		  "=c" (data[2]),
		  "=d" (data[3])
		 : "a"(op1),
           "c"(op2));
}

static inline uint64_t
rdmsr64(uint32_t msr)
{
    uint32_t lo = 0;
    uint32_t hi = 0;
    uint64_t val = 0;

    rdmsr(msr, lo, hi);

    val = (((uint64_t)hi) << 32) | ((uint64_t)lo);

#ifdef DEBUG_KEXT
    printf("rdmsr64(0x%.16lX) => %.16llX\n", (unsigned long)msr, (unsigned long long)val);
#endif /* DEBUG_KEXT */

    return val;
}

static inline void wrmsr64(UInt32 msr, UInt64 val)
{
    UInt32 lo = ((UInt32)(val & 0xFFFFFFFF));
    UInt32 hi = ((UInt32)((val & 0xFFFFFFFF00000000) >> 32));

#ifdef DEBUG_KEXT
    printf("wrmsr64(0x%.16lX, %.16llX)\n", (unsigned long)msr, (unsigned long long)val);
#endif /* DEBUG_KEXT */

    wrmsr(msr, lo, hi);
}

void
DirectHWUserClient::MSRHelperFunction(void *data)
{
	MSRHelper *MSRData = (MSRHelper *)data;
	msrcmd_t *inStruct = MSRData->in;
	msrcmd_t *outStruct = MSRData->out;

	outStruct->core = ((UInt32)-1);

	outStruct->val.io32.lo = INVALID_MSR_LO;
	outStruct->val.io32.hi = INVALID_MSR_HI;

	uint32_t cpuiddata[4];

	cpuid(1, 0, cpuiddata);

	//bool have_ht = ((cpuiddata[3] & (1 << 28)) != 0);

	uint32_t core_id = cpuiddata[1] >> 24;

	cpuid(11, 0, cpuiddata);

	uint32_t smt_mask = ~((-1) << (cpuiddata[0] &0x1f));

	// TODO: What we want is this:
	// if (inStruct->core != cpu_to_core(cpu_number()))
	//	return;

	if ((core_id & smt_mask) != core_id)
    {
		return; // It's a HT thread
    }

	if (inStruct->core != cpu_number())
    {
		return;
    }

	IOLog("DirectHW: ReadMSRHelper %ld %ld %lx \n",
          (long)inStruct->core, (long)cpu_number(), (unsigned long)smt_mask);

	if (MSRData->Read)
    {
        uint64_t ret = rdmsr64(inStruct->index);

        outStruct->val.io64 = ret;
	} else {
        wrmsr64(inStruct->index, inStruct->val.io64);
	}

	outStruct->index = inStruct->index;
	outStruct->core = inStruct->core;
}

IOReturn
DirectHWUserClient::ReadMSRAsync(OSAsyncReference asyncRef,
                                 msrcmd_t *inStruct, msrcmd_t *outStruct,
                                 IOByteCount inStructSize,
                                 IOByteCount *outStructSize)
{
    ((void)asyncRef);

    return DirectHWUserClient::ReadMSR(inStruct, outStruct, inStructSize, outStructSize);
}

IOReturn
DirectHWUserClient::ReadMSR(msrcmd_t *inStruct, msrcmd_t *outStruct,
                            IOByteCount inStructSize,
                            IOByteCount *outStructSize)
{
    ((void)inStructSize);

	if ((fProvider == NULL) || (isInactive()))
    {
		return kIOReturnNotAttached;
	} 

	MSRHelper MSRData = { inStruct, outStruct, true };

#ifdef USE_MP_RENDEZVOUS
	mp_rendezvous(NULL, (void (*)(void *))MSRHelperFunction, NULL, (void *)&MSRData);
#else /* !USE_MP_RENDEZVOUS */
    mp_rendezvous_no_intrs((void (*)(void *))MSRHelperFunction, (void *)&MSRData);
#endif /* USE_MP_RENDEZVOUS */

    if (outStructSize != NULL)
    {
        *outStructSize = sizeof(msrcmd_t);
    }

	if (outStruct->core != inStruct->core)
    {
		return kIOReturnIOError;
    }

#ifdef DEBUG_KEXT
	IOLog("DirectHW: ReadMSR(0x%16lx) => 0x%16llx\n",
          (unsigned long)inStruct->index, (unsigned long long)outStruct->val.io64);
#endif /* DEBUG_KEXT */

	return kIOReturnSuccess;
}

IOReturn
DirectHWUserClient::WriteMSRAsync(OSAsyncReference asyncRef,
                                  msrcmd_t *inStruct, msrcmd_t *outStruct,
                                  IOByteCount inStructSize,
                                  IOByteCount *outStructSize)
{
    ((void)asyncRef);

    return DirectHWUserClient::WriteMSR(inStruct, outStruct, inStructSize, outStructSize);
}

IOReturn
DirectHWUserClient::WriteMSR(msrcmd_t *inStruct, msrcmd_t *outStruct,
                             IOByteCount inStructSize,
                             IOByteCount *outStructSize)
{
    ((void)inStructSize);

	if ((fProvider == NULL) || (isInactive()))
    {
		return kIOReturnNotAttached;
	} 

#ifdef DEBUG_KEXT
	IOLog("DirectHW: WriteMSR(0x%16lx) = 0x%16llx\n",
          (unsigned long)inStruct->index, (unsigned long long)inStruct->val.io64);
#endif /* DEBUG_KEXT */

	MSRHelper MSRData = { inStruct, outStruct, false };

#ifdef USE_MP_RENDEZVOUS
	mp_rendezvous(NULL, (void (*)(void *))MSRHelperFunction, NULL, (void *)&MSRData);
#else /* !USE_MP_RENDEZVOUS */
    mp_rendezvous_no_intrs((void (*)(void *))MSRHelperFunction, (void *)&MSRData);
#endif /* USE_MP_RENDEZVOUS */

    if (outStructSize != NULL)
    {
        *outStructSize = sizeof(msrcmd_t);
    }

	if (outStruct->core != inStruct->core)
    {
		return kIOReturnIOError;
    }

	return kIOReturnSuccess;
}

IOReturn DirectHWUserClient::clientMemoryForType(UInt32 type, UInt32 *flags, IOMemoryDescriptor **memory)
{
	IOMemoryDescriptor *newmemory = NULL;

#ifndef DEBUG_KEXT
    ((void)flags);
#else
	IOLog("DirectHW: clientMemoryForType(%lx, %p, %p)\n",
          (unsigned long)type, (void *)flags, (void *)memory);
#endif /* DEBUG_KEXT */

	if (type != 0)
    {
		IOLog("DirectHW: Unknown mapping type %lx.\n", (unsigned long)type);

		return kIOReturnUnsupported;
	}

	if ((LastMapAddr == 0) && (LastMapSize == 0))
    {
		IOLog("DirectHW: No PrepareMap called.\n");

		return kIOReturnNotAttached;
	}

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Mapping physical 0x%16lx[0x%lx]\n",
          (unsigned long)LastMapAddr, (unsigned long)LastMapSize);
#endif /* DEBUG_KEXT */

    if (memory != NULL)
    {
        newmemory = IOMemoryDescriptor::withPhysicalAddress(LastMapAddr, LastMapSize, kIODirectionIn);
    }
	
	/* Reset mapping to zero */
	LastMapAddr = 0;
	LastMapSize = 0;

	if (newmemory == NULL)
    {
		IOLog("DirectHW: Could not map memory!\n");

		return kIOReturnNotOpen;
	}

	newmemory->retain();

    if (memory != NULL)
    {
        *memory = newmemory;
    }

#ifdef DEBUG_KEXT
	IOLog("DirectHW: Mapping succeeded.\n");
#endif /* DEBUG_KEXT */

	return kIOReturnSuccess;
}
