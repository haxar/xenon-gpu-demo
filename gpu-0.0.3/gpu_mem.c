#include "xenos.h"
#include <stdio.h>
#include <stdlib.h>

#define CACHELINE_SIZE 128

#define __dcbst(where) asm volatile ("dcbst 0,%0" : : "r"(where) : "memory")
#define __dcbf(where) asm volatile ("dcbf 0,%0" : : "r"(where) : "memory")
#define __sync() asm volatile ("sync" : : : "memory")


void MEM_SyncToDevice(volatile void *data, int len)
{
	while (len > 0)
	{
		__dcbst(data);
		data += CACHELINE_SIZE;
		len -= CACHELINE_SIZE;
	}
}

void MEM_SyncFromDevice(volatile void *data, int len)
{
	while (len > 0)
	{
		__dcbf(data);
		data += CACHELINE_SIZE;
		len -= CACHELINE_SIZE;
	}
}

static int alloc_ptr = 0;

void *MEM_Alloc(u32 *phys, int size, int align)
{
	void *r;
	if (!align)
		align = size;
	alloc_ptr += (-alloc_ptr) & (align-1);
	alloc_ptr += align;
	r = ((void*)rb) + alloc_ptr;

	if (phys)
		*phys = RINGBUFFER_BASE + alloc_ptr;
	alloc_ptr += size;
	if (alloc_ptr > (RINGBUFFER_SIZE))
	{
		fprintf(stderr, "FATAL: out of memory.\n");
		exit(0);
	}
	return r;
}

