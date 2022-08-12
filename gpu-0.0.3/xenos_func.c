#include "xenos.h"
#include <stdio.h>

void InvalidateGpuCache(int base, int size)
{
	rput32(0x00000a31);
		rput32(0x01000000);
	rput32(0x00010a2f);
		rput32(size); rput32(base);
	rput32(0xc0043c00);
		rput32(0x00000003); rput32(0x00000a31); rput32(0x00000000); rput32(0x80000000); rput32(0x00000008);
}

void InvalidateGpuCacheAll(int base, int size)
{
	rput32(0x00000a31);
		rput32(0x03000100);
	rput32(0x00010a2f);
		rput32(size); rput32(base);
	rput32(0xc0043c00);
		rput32(0x00000003); rput32(0x00000a31); rput32(0x00000000); rput32(0x80000000); rput32(0x00000008);
}

void SetSurfaceClip(int offset_x, int offset_y, int sc_left, int sc_top, int sc_right, int sc_bottom)
{
	rput32(0x00022080);
		rput32(xy32(offset_x, offset_y));
		rput32(xy32(sc_left, sc_top));
		rput32(xy32(sc_right, sc_bottom));
}

void SetBin(u32 mask_low, u32 select_low, u32 mask_hi, u32 select_hi)
{
	rput32(0xc0006000);
		rput32(mask_low);
	rput32(0xc0006200);
		rput32(select_low);
	rput32(0xc0006100);
		rput32(mask_hi);
	rput32(0xc0006300);
		rput32(select_hi); 
}

void WaitUntilIdle(u32 what)
{
	rput32(0x000005c8);
		rput32(what);
}


void DrawNonIndexed(int num_points)
{
	rput32(0xc0012201);
		rput32(0x00000000); rput32(0x00000084 | (num_points << 16));
}

void SetIndexOffset(int offset)
{
	rput32(0x00002102);
		rput32(0x00000000);
}

void VERTEX_FETCH(u32 *dst, u32 base, int len)
{
	dst[0] = base | 3;
	dst[1] = 0x10000002 | (len << 2);
}

void TEXTURE_FETCH(u32 *dst, u32 base, int width, int height, int pitch, int tiled, int format, u32 base_mip, int anisop)
{
	dst[0] = 0x00000002 | (pitch << 22) | (tiled << 31);
	dst[1] = 0x00000000 | base | format; /* BaseAddress */
	dst[2] = (height << 13) | width;
	dst[3] = 0x00a80c14 | (anisop << 25);
	if (base_mip)
		dst[4] = 0x00000e03;
	else
		dst[4] = 0;
	dst[5] = 0x00000a00 | base_mip; /* MipAddress */
}

#define SHADER_TYPE_PIXEL 1
#define SHADER_TYPE_VERTEX 0

void LoadShader(int base, int type, int size)
{
	rput32(0xc0012700);
		rput32(base | type); 
		rput32(size);
}

void align(void)
{
	while ((rb_secondary_wptr&3) != 3)
		rput32(0x80000000);
}

void block_until_idle(void)
{
	write_reg(0x1720, 0x20000);
}

void step(u32 x)
{
	write_reg(0x15e0, x);
}

void InvalidateGpuCache_Primary(int base, int size)
{
	rput32p(0x00000a31); 
		rput32p(0x01000000);
	rput32p(0x00010a2f); 
		rput32p(size); rput32p(base);
	rput32p(0xc0043c00); 
		rput32p(0x00000003); rput32p(0x00000a31); rput32p(0x00000000); rput32p(0x80000000); rput32p(0x00000008);
}


void _stuff(void)
{
	SetSurfaceClip(0, 0, 0, 0, x.vp_xres, x.vp_yres);

	rput32(0x00012000); 
		rput32(SurfaceInfo(0x410, 0, 0x400)); 
		rput32(0x00000000); 
	rput32(0x0000200d); 
		rput32(0x00000000); 
	rput32(0x00012100); 
		rput32(0x00ffffff); rput32(0x00000000); 
	rput32(0x00002104); 
		rput32(0x0000000f);
	rput32(0x0000210d); 
		rput32(0x00ffff00); 

	rput32(0x00002301);
		rput32(0x00000000); 
	rput32(0x00002312);
		rput32(0x0000ffff); 
	rput32(0x00072380);
		rput32(0x00000000); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000);
		rput32(0x00000000); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000);
		
}
