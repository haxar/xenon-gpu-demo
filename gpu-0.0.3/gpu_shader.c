#include "xenos.h"
#include <stdio.h>
#include <stdlib.h>

	/* shader file format */
struct SH_Header
{
	u32 magic;
	u32 offset;
	
	u32 _[3];

	u32 off_constants, off_shader;
};

struct SH_Data
{
	u32 sh_off, sh_size;
	u32 program_control, context_misc;
	u32 _[2];
};

struct SH_Vertex
{
	u32 cnt0, cnt_vfetch, cnt2;
};

struct Shader *SH_Load(const char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f)
	{
		fprintf(stderr, "FATAL: shader %s not found!\n", filename);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	void *m = malloc(size);
	fread(m, size, 1, f);
	fclose(f);
	
	struct SH_Header *hdr = m;
	
	if ((hdr->magic >> 16) != 0x102a)
	{
		fprintf(stderr, "shader version: %08x, expected: something with 102a.\n", hdr->magic);
		exit(1);
	}

	struct Shader *s = malloc(sizeof(struct Shader));
	s->shader = m;
	s->shader_phys = 0;

	struct SH_Data *data = m + hdr->off_shader;
	s->program_control = data->program_control;
	s->context_misc = data->context_misc;

	return s;
}

void SH_Upload_Constants(struct Shader *s)
{
	struct SH_Header *hdr = s->shader;

	if (hdr->off_constants)
	{
			/* upload shader constants */
		void *constants = s->shader + hdr->off_constants;
	
		constants += 16;
	
		int size = *(u32*)constants; constants += 4;
		size -= 0xC;
	
		while (size)
		{
			u16 start = *(u16*)constants; constants += 2;
			u16 count = *(u16*)constants; constants += 2;
			u32 offset = *(u32*)constants; constants += 4;
			
			float *c = s->shader + hdr->offset + offset;
			memcpy(x.alu_constants + start * 4, c, count * 4);
			DirtyAluConstant(start, 4);

			size -= 8;
		}
	}
}

int SH_VBF_CalcSize(const struct VBF_Element *fmt)
{
	switch (fmt->fmt)
	{
	case 6: // char4
		return 4;
	case 37: // float2
		return 8;
	case 38: // float4
		return 16;
	case 57: // float3
		return 12;
	default:
		fprintf(stderr, "Unknown VBF %d!\n", fmt->fmt);
		exit(1);
	}
}

int SH_VBF_CalcStride(const struct VBF_Format *fmt)
{
	int i;
	int total_size = 0;
	for (i=0; i<fmt->num; ++i)
		total_size += SH_VBF_CalcSize(&fmt->e[i]);
	return total_size;
}

	/* shaders are not specific to a vertex input format.
	   the vertex format specified in a vertex shader is just
	   dummy. Thus we need to patch the vfetch instructions to match
	   our defined structure. */
void SH_VBF_ApplyVFetchPatches(struct Shader *sh, int index, const struct VBF_Format *fmt)
{
	int stride = SH_VBF_CalcStride(fmt);
	
	if (stride & 3)
	{
		fprintf(stderr, "your vertex buffer format is not DWORD aligned.\n");
		exit(1);
	}
	
	stride /= 4;

	struct SH_Header *hdr = sh->shader;
	printf("off_shader: %d\n", hdr->off_shader);
	struct SH_Data *data = sh->shader + hdr->off_shader;
	printf("data: %08x, %08x\n", data->sh_off, data->sh_size);
	
	void *shader_code = sh->shader + data->sh_off + hdr->offset;
	u32 *c = (u32*)(data + 1);
	int skip = *c++;
	int num_vfetch = *c;
	++c;
	
	c += skip * 2;
	int i;
	
	int fetched_to = 0;

	for (i=0; i<num_vfetch; ++i)
	{
		u32 vfetch_patch = *c++;
		int type = (vfetch_patch >> 12) & 0xF;
		int stream = (vfetch_patch >> 16) & 0xF;
		int insn = vfetch_patch & 0xFFF;
		
		printf("type=%d, stream=%d, insn=%d\n", type, stream, insn);
		u32 *vfetch = shader_code + insn * 12;
		printf("  old vfetch: %08x %08x %08x\n", vfetch[0], vfetch[1], vfetch[2]);
		
		int Offset = (vfetch[2] & 0x7fffff00) >> 8;
		int DataFormat = (vfetch[1] & 0x003f0000) >> 16;
		int Stride= (vfetch[2] & 0x000000ff);
		int Signed= (vfetch[1] & 0x00001000) >> 12;
		int NumFormat = (vfetch[1] & 0x00002000) >> 13;
		int PrefetchCount= (vfetch[0] & 0x38000000) >> 27;
		printf("  old Offset=%08x, DataFormat=%d, Stride=%d, Signed=%d, NumFormat=%d, PrefetchCount=%d\n",
			Offset,DataFormat, Stride, Signed, NumFormat, PrefetchCount);
		
		if (index != stream)
		{
			printf("  (not for this vertex stream)\n");
			continue;
		}

			/* let's find the element which applies for this. */
		int j;
		int offset = 0;
		for (j=0; j < fmt->num; ++j)
		{
			if (fmt->e[j].usage == type)
				break;
			offset += SH_VBF_CalcSize(&fmt->e[j]);
		}
		
		offset /= 4;
		
		if (j == fmt->num)
		{
			fprintf(stderr, "shader requires input type %d, which wasn't found in vertex format.\n", type);
			exit(1);
		}

		Offset = offset;
		DataFormat = fmt->e[j].fmt;
		
		Signed = 0;
		Stride = stride;
		NumFormat = 0; // fraction

		if (DataFormat != 6)
			NumFormat = 1;
		
		int to_fetch = 0;

			/* if we need fetching... */
		if (fetched_to <= offset + ((SH_VBF_CalcSize(&fmt->e[j])+3)/4))
			to_fetch = stride - fetched_to;

		if (to_fetch > 8)
			to_fetch = 8;

		int is_mini = 0;

		if (to_fetch == 0)
		{
			PrefetchCount = 0;
			is_mini = 1;
		} else
			PrefetchCount = to_fetch - 1;

		fetched_to += to_fetch;

			/* patch vfetch instruction */
		vfetch[0] &= ~(0x00000000|0x00000000|0x00000000|0x00000000|0x00000000|0x38000000|0x00000000|0x00000000);
		vfetch[1] &= ~(0x00000000|0x003f0000|0x00000000|0x00001000|0x00002000|0x00000000|0x40000000|0x00000FFF);
		vfetch[2] &= ~(0x7fffff00|0x00000000|0x000000ff|0x00000000|0x00000000|0x00000000|0x00000000|0x00000000);

		vfetch[2] |= Offset << 8;
		vfetch[1] |= DataFormat << 16;
		vfetch[2] |= Stride;
		vfetch[1] |= Signed << 12;
		vfetch[1] |= NumFormat << 13;
		vfetch[0] |= PrefetchCount << 27;
		vfetch[1] |= is_mini << 30;
		vfetch[1] |= fmt->e[j].swizzle;

		Offset = (vfetch[2] & 0x7fffff00) >> 8;
		DataFormat = (vfetch[1] & 0x003f0000) >> 16;
		Stride= (vfetch[2] & 0x000000ff);
		Signed= (vfetch[1] & 0x00001000) >> 12;
		NumFormat = (vfetch[1] & 0x00002000) >> 13;
		PrefetchCount= (vfetch[0] & 0x38000000) >> 27;
		printf("  new Offset=%08x, DataFormat=%d, Stride=%d, Signed=%d, NumFormat=%d, PrefetchCount=%d\n",
			Offset,DataFormat, Stride, Signed, NumFormat, PrefetchCount);
	}
	
}

void SH_InitShader(struct Shader *sh)
{
	struct SH_Header *hdr = sh->shader;
	struct SH_Data *data = sh->shader + hdr->off_shader;
	void *shader_code = sh->shader + data->sh_off + hdr->offset;
	
	sh->shader_phys_size = data->sh_size;
	printf("allocating %d bytes\n", data->sh_size);
	void *p = MEM_Alloc(&sh->shader_phys, data->sh_size, 0x100);
	memcpy(p, shader_code, data->sh_size);
	MEM_SyncToDevice(p, data->sh_size);
}
