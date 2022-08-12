#include "xenos.h"

struct XenosState x;

int stat_alu_uploaded = 0;

static void UploadALUConstants(void)
{
	while (x.alu_dirty)
	{
		int start, end;
		for (start = 0; start < 32; ++start)
			if (x.alu_dirty & (1<<start))
				break;
		for (end = start; end < 32; ++end)
			if (!(x.alu_dirty & (1<<end)))
				break;
			else
				x.alu_dirty &= ~(1<<end);
		
		int base = start * 16;
		int num = (end - start) * 16 * 4;
		
		stat_alu_uploaded += num;
		align();
		rput32(0x00004000 | (base * 4) | ((num-1) << 16));
			rput(x.alu_constants + base * 4, num);
	}
}

static void UploadFetchConstants(void)
{
	while (x.fetch_dirty)
	{
		int start, end;
		for (start = 0; start < 32; ++start)
			if (x.fetch_dirty & (1<<start))
				break;
		for (end = start; end < 32; ++end)
			if (!(x.fetch_dirty & (1<<end)))
				break;
			else
				x.fetch_dirty &= ~(1<<end);
		
		int base = start * 3;
		int num = (end - start) * 3 * 2;
		
		stat_alu_uploaded += num;
		align();
		rput32(0x00004800 | (base * 2) | ((num-1) << 16));
			rput(x.fetch_constants + base * 2, num);
	}
}

static void UploadClipPlane(void)
{
	align();
	rput32(0x00172388);
		rput(x.clipplane, 6*4);
}

static void UploadIntegerConstants(void)
{
	align();
	rput32(0x00274900);
		rput(x.integer_constants, 10*4);
}

static void UploadControl(void)
{
	rput32(0x00082200);
		rput(x.controlpacket, 9);
}

static void UploadShader(void)
{
	u32 program_control = 0, context_misc = 0;
	if (x.ps)
	{
		LoadShader(x.ps->shader_phys, SHADER_TYPE_PIXEL, x.ps->shader_phys_size);
		SH_Upload_Constants(x.ps);
		program_control |= x.ps->program_control;
		context_misc |= x.ps->context_misc;
	}

	if (x.vs)
	{
		LoadShader(x.vs->shader_phys, SHADER_TYPE_VERTEX, x.vs->shader_phys_size);
		SH_Upload_Constants(x.vs);
		program_control |= x.vs->program_control;
		context_misc |= x.vs->context_misc;
	}

	rput32(0x00022180);
		rput32(program_control);
		rput32(context_misc);
		rput32(0x00010001); 
}

void InitControl(void)
{
	x.controlpacket[0] = 0x00700736;
	x.controlpacket[1] = 0x00010001;
	x.controlpacket[2] = 0x87000007;
	x.controlpacket[3] = 0x00000000;
	x.controlpacket[4] = 0x00080000;
	x.controlpacket[5] = 0x00018006;
	x.controlpacket[6] = 0x0000043f;
	x.controlpacket[7] = 0;
	x.controlpacket[8] = 0x00000004;
	
	x.dirty |= DIRTY_CONTROL;
}

void SetDepthControl(int z_func)
{
	x.controlpacket[0] = (x.controlpacket[0]&~0xF0) | (z_func<<4);
	x.dirty |= DIRTY_CONTROL;
}

void SetZWrite(int zw)
{
	x.controlpacket[0] = (x.controlpacket[0]&~2) | (zw<<1);
	x.dirty |= DIRTY_CONTROL;
}

void SetFillMode(int front, int back)
{
	x.controlpacket[5] &= ~(0x3f<<5);
	x.controlpacket[5] |= front << 5;
	x.controlpacket[5] |= back << 8;
	x.controlpacket[5] |= 1<<3;

	x.dirty |= DIRTY_CONTROL;
}

void SetBlendControl(int col_src, int col_op, int col_dst, int alpha_src, int alpha_op, int alpha_dst)
{
	x.controlpacket[1] = col_src | (col_op << 5) | (col_dst << 8) | (alpha_src << 16) | (alpha_op << 21) | (alpha_dst << 24);
	x.dirty |= DIRTY_CONTROL;
}

void InvalidateState(void)
{
	x.dirty = ~0;
	x.alu_dirty = ~0;
	x.fetch_dirty = ~0;
	InitControl();
}

void SetShader(int type, struct Shader *sh)
{
	struct Shader **s;
	if (type == SHADER_TYPE_PIXEL)
		s = &x.ps;
	else
		s = &x.vs;
	
	if (*s != sh)
	{
		*s = sh;
		x.dirty |= DIRTY_SHADER;
	}
}

void SetState(void)
{
	if (x.dirty & DIRTY_ALU)
		UploadALUConstants();
	
	if (x.dirty & DIRTY_FETCH)
	{
		UploadFetchConstants();
		rput32(0x00025000);
			rput32(0x00000000); rput32(0x00025000); rput32(0x00000000); 
	}
	
	if (x.dirty & DIRTY_CLIP)
		UploadClipPlane();
	
	if (x.dirty & DRITY_INTEGER)
		UploadIntegerConstants();
	
	if (x.dirty & DIRTY_CONTROL)
		UploadControl();

	if (x.dirty & DIRTY_SHADER)
		UploadShader();

	x.dirty = 0;
}

void DirtyAluConstant(int base, int len)
{
	len += base & 15;
	base >>= 4;
	while (len > 0)
	{
		x.alu_dirty |= 1 << base;
		++base;
		len -= 16;
	}
	x.dirty |= DIRTY_ALU;
}

void DirtyFetch(int base, int len)
{
	len += base % 3;
	base /= 3;
	while (len > 0)
	{
		x.fetch_dirty |= 1 << base;
		++base;
		len -= 3;
	}
	x.dirty |= DIRTY_FETCH;
}

void SetTexture(int index, struct Texture *tex)
{
	TEXTURE_FETCH(x.fetch_constants + index * 2, tex->ptr, tex->width - 1, tex->height - 1, tex->pitch, tex->tiled, tex->format, tex->ptr_mip, 2);
	DirtyFetch(index, 3);
}

