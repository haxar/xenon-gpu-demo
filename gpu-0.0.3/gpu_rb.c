#include "xenos.h"

static int last_wptr;

void RB_CommitPrimary(void)
{
	int i;
	for (i=0; i<0x20; ++i)
		rput32p(0x80000000);
	MEM_SyncToDevice(rb_primary, RINGBUFFER_PRIMARY_SIZE * 4);
	__asm__ (" sync");
	w32(0x0714, rb_primary_wptr);
//	printf("committed to %08x\n", rb_primary_wptr);
}

void RB_KickSegment(int base, int len)
{
//	printf("kick_segment: %x, len=%x\n", base, len * 4);
	MEM_SyncToDevice(rb_secondary + base * 4, len * 4);
	InvalidateGpuCache_Primary(rb_secondary_base + base * 4, len * 4 + 0x1000);
	rput32p(0xc0013f00);
		rput32p(rb_secondary_base + base * 4); rput32p(len);
}

#define RINGBUFFER_SECONDARY_GUARD 0x20000

void RB_Kick(void)
{
//	printf("kick: wptr = %x, last_wptr = %x\n", rb_secondary_wptr, last_wptr);
	
	RB_KickSegment(last_wptr, rb_secondary_wptr - last_wptr);

	rb_secondary_wptr += (-rb_secondary_wptr)&0x1F; /* 128byte align */
	
	if (rb_secondary_wptr > (RINGBUFFER_SECONDARY_SIZE - RINGBUFFER_SECONDARY_GUARD))
		rb_secondary_wptr = 0;
	
	last_wptr = rb_secondary_wptr;

	RB_CommitPrimary();
}

#define SEGMENT_SIZE 1024

void RB_MayKick(void)
{
//	printf("may kick: wptr = %x, last_wptr = %x\n", rb_secondary_wptr, last_wptr);
	int distance = rb_secondary_wptr - last_wptr;
	if (distance < 0)
		distance += RINGBUFFER_SECONDARY_SIZE;
	
	if (distance >= SEGMENT_SIZE)
		RB_Kick();
}

u32 RB_Alloc(void)
{
	u32 rb_primary_phys;
	rb_primary = MEM_Alloc(&rb_primary_phys, RINGBUFFER_PRIMARY_SIZE * 4, 0);
	rb_secondary = MEM_Alloc(&rb_secondary_base, RINGBUFFER_SECONDARY_SIZE * 4, 0x100);
	return rb_primary_phys;
}
