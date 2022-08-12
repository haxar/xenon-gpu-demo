#include "xenos.h"
#include <stdio.h>
#include <stdlib.h>

static struct VertexBuffer *vb_pool;
static struct VertexBuffer *vb_pool_after_frame;

struct VertexBuffer *VB_Alloc(int size)
{
	struct VertexBuffer *vb = malloc(sizeof(struct VertexBuffer));
	printf("--- alloc new vb, at %p\n", vb);
	vb->base = MEM_Alloc(&vb->phys_base, size * 4, 0x100);
	vb->size = 0;
	vb->space = size;
	vb->next = 0;
	vb->vertices = 0;
	return vb;
}

struct VertexBuffer *VB_PoolAlloc(int size)
{
	struct VertexBuffer **vbp = &vb_pool;
	
	while (*vbp)
	{
		struct VertexBuffer *vb = *vbp;
		if (vb->space >= size)
		{
			*vbp = vb->next;
			vb->next = 0;
			vb->size = 0;
			vb->vertices = 0;
			return vb;
		}
		vbp = &vb->next;
	}
	
	return VB_Alloc(size);
}

void VB_PoolAdd(struct VertexBuffer *vb)
{
	struct VertexBuffer **vbp = vb_pool_after_frame ? &vb_pool_after_frame->next : &vb_pool_after_frame;
	while (*vbp)
		vbp = &(*vbp)->next;

	*vbp = vb;
}

void VB_Reclaim(void)
{
	struct VertexBuffer **vbp = vb_pool ? &vb_pool->next : &vb_pool;
	while (*vbp)
		vbp = &(*vbp)->next;
	
	*vbp = vb_pool_after_frame;
	vb_pool_after_frame = 0;
}

static struct VertexBuffer *vb_current, *vb_head;
static int vb_current_pitch;

void VB_Begin(int pitch)
{
	if (vb_head || vb_current)
	{
		fprintf(stderr, "FATAL: VertexBegin without VertexEnd!\n");
		exit(1);
	}
	vb_current_pitch = pitch;
}

void VB_Put(void *data, int len)
{
	if (len % vb_current_pitch)
	{
		fprintf(stderr, "FATAL: VertexPut with non-even len\n");
		exit(1);
	}
	
	while (len)
	{
		int remaining = vb_current ? (vb_current->space - vb_current->size) : 0;
		
		remaining -= remaining % (vb_current_pitch *6);
		
		if (remaining > len)
			remaining = len;
		
		if (!remaining)
		{
			struct VertexBuffer **n = vb_head ? &vb_current->next : &vb_head;
			vb_current = VB_PoolAlloc(0x10000);
			*n = vb_current;
			continue;
		}
		
		memcpy(vb_current->base + vb_current->size * 4, data, remaining * 4);
		vb_current->size += remaining;
		vb_current->vertices += remaining / vb_current_pitch;
		data += remaining * 4;
		len -= remaining;
	}
}

struct VertexBuffer *VB_End(void)
{
	struct VertexBuffer *res;
	res = vb_head;
	
	while (vb_head)
	{
		InvalidateGpuCache(vb_head->phys_base, (vb_head->space * 4) + 0x1000);
		MEM_SyncToDevice(vb_head->base, vb_head->space * 4);
		vb_head = vb_head->next;
	}

	vb_head = vb_current = 0;

	return res;
}
