#include "xenos.h"

static u32 resolve_vb_phys;

void RS_Resolve(struct Texture *target)
{
	SetSurfaceClip(0, 0, 0, 0, target->width, target->height);

	rput32(0x00012000); 
		rput32(0x10000410); rput32(0x00000000); 
	rput32(0x00002104); 
		rput32(0x0000000f); 
	rput32(0x0005210f); 
		rput32(0x44000000); rput32(0x44000000); 
		rput32(0xc3b40000); rput32(0x43b40000); 
		rput32(0x3f800000); rput32(0x00000000); 

	rput32(0x00002301); 
		rput32(0x00000000); 

	rput32(0x00032318); 
		rput32(0x00100300); // 300 = color,depth clear enabled!
		rput32(target->ptr);
		rput32(xy32(target->pitch * 32, target->height));
		rput32(0x01000300);

	write_reg(0x8c74, 0xffffffff); // ??
	write_reg(0x8c78, 0x800080ff);

	rput32(0xc0003b00); rput32(0x00000100);

	rput32(0xc0102b00); rput32(0x00000000);
		rput32(0x0000000f); rput32(0x10011002); rput32(0x00001200); rput32(0xc4000000);
		rput32(0x00000000); rput32(0x1003c200); rput32(0x22000000); rput32(0x00080000);
		rput32(0x00253b48); rput32(0x00000002); rput32(0xc80f803e); rput32(0x00000000);
		rput32(0xc2000000); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000);

	rput32(0x00012180); rput32(0x00010002); rput32(0x00000000);
	rput32(0x00002208); rput32(0x00000006); 

	rput32(0x00002200); rput32(0x00008777); 

	rput32(0x000005c8); rput32(0x00020000); 
	rput32(0x00002203); rput32(0x00000000); 
	rput32(0x00022100); rput32(0x0000ffff); rput32(0x00000000); rput32(0x00000000); 
	rput32(0x00022204); rput32(0x00010000); rput32(0x00010000); rput32(0x00000300); 
	rput32(0x00002312); rput32(0x0000ffff); 
	rput32(0x0000200d); rput32(0x00000000); 

	rput32(0x00054800); rput32((resolve_vb_phys) | 3); rput32(0x1000001a); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000);
	rput32(0x00025000); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000);

	rput32(0xc0003600); rput32(0x00030088); 

	rput32(0xc0004600); rput32(0x00000006); 
	rput32(0x00002007); rput32(0x00000000); 
	InvalidateGpuCacheAll(target->ptr, target->pitch * target->height * 32 * 4);

	rput32(0x0000057e); rput32(0x00010001); 
	rput32(0x00002318); rput32(0x00000000);
	rput32(0x0000231b); rput32(0x00000000);
	rput32(0x00001844); rput32(target->ptr); 
	rput32(0xc0022100); rput32(0x00001841); rput32(0xfffff8ff); rput32(0x00000000);
	rput32(0x00001930); rput32(0x00000000);
	rput32(0xc0003b00); rput32(0x00007fff);

	rput32(0xc0025800); rput32(0x00000003);
		rput32(0x1fc4e006); rput32(0xbfb75313);
	rput32(0xc0025800); rput32(0x00000003);
		rput32(0x1fc4e002); rput32(0x000286d1);
}

void RS_Init(int xres, int yres)
{
			/* VB used for edram -> fb copy */
			/* (we should use the VB_* functions here, but this function is older than the VB stuff) */
	float *resolve_vb = MEM_Alloc(&resolve_vb_phys, 6*4, 0x100);
	resolve_vb[0 / 4] = -.5;
	resolve_vb[4 / 4] = -.5;
	resolve_vb[8 / 4] = xres - .5;
	resolve_vb[0xc / 4] = 0;
	resolve_vb[0x10 / 4] = xres - .5;
	resolve_vb[0x14 / 4] = yres - .5;
}
