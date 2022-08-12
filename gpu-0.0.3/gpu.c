#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <byteswap.h>
#include <math.h>
#include <signal.h>

#include "xenos.h"

#include "engine.h"

volatile void * ioremap(unsigned long physaddr, unsigned size, int sync);

volatile unsigned int *regs;

unsigned int r32(int offset)
{
  return regs[offset/4];
}

		/* reasonable defaults, will be overwritten */
#define XRES 1024 
#define YRES 720

void w32(int offset, unsigned int val)
{
  regs[offset/4] = val;
}

volatile void *rb, *rb_primary, *rb_secondary;
int rb_primary_wptr, rb_secondary_wptr;
u32 rb_secondary_base;

#define FB_ADDR 0x1f6b0000

struct Texture tex_test = {256, 256, 8, 0, FORMAT_8888 | FORMAT_ARGB};
struct Texture tex_fb =  {XRES, YRES, XRES/32, 1, FORMAT_8888, FB_ADDR};

long long tris_drawn;

void draw(struct VertexBuffer *vb)
{
	_stuff();
	int a = 0;
	
//	vb = vb->next;

	while (vb)
	{
		VERTEX_FETCH(x.fetch_constants + 95 * 2, vb->phys_base, vb->size);
		DirtyFetch(95, 1);

		SetState();

		rput32(0x00002007);
		rput32(0x00000000);

		SetIndexOffset(0);
		DrawNonIndexed(vb->vertices);
		tris_drawn += vb->vertices / 3;
		vb = vb->next;
		a++;
	}
}

int loadbin(void *addr, const char *filename, int max)
{
	FILE *f = fopen(filename, "rb");
	if (!f)
	{
		fprintf(stderr, "couldn't open %s: %m\n", filename);
		abort();
	}
	int r = fread(addr, 1, max, f);
	fclose(f);
	return r;
}

u32 loadbin_phys_skip(const char *filename, int align, int skip)
{
	FILE *f = fopen(filename, "rb");
	if (!f)
	{
		fprintf(stderr, "couldn't open %s: %m\n", filename);
		abort();
	}
	fseek(f, 0, SEEK_END);
	int size = ftell(f) - skip;
	fseek(f, skip, SEEK_SET);
	
	u32 ptr;
	void *b = MEM_Alloc(&ptr, size, align);

	if (fread(b, 1, size, f) != size)
	{
		fprintf(stderr, "fatal error: read error %s\n", filename);
		exit(1);
	}
	MEM_SyncToDevice(b, size);
	fclose(f);
	return ptr;
}


u32 loadbin_phys(const char *filename, int align)
{
	return loadbin_phys_skip(filename, align, 0);
}

void dump(int sig)
{
	printf("dumped..\n");
	FILE *f = fopen("ringbuffer_dump", "w");
	fwrite((void*)rb_primary, 1, 0x200000, f);
	fclose(f);
	exit(0);
}

void genTorusFnc2(eVector3 v, float u)
{
	float q = 3, p = 5;
	u *= p;
	v[0] = (3 + cos((q * u) / p)) * cos(u);
	v[1] = (3 + cos((q * u) / p)) * sin(u);
	v[2] = sin((q * u) / p);
}

const float STEP_U = .001, STEP_V = .05;

void genTorusFnc(eVector3 vec, float u, float v)
{
	float rad = 0.1;
	u *= 2.0 * M_PI;
	v *= 2.0 * M_PI;
	
	eVector3 center;
	genTorusFnc2(center, u);
	eVector3 dpos;
	genTorusFnc2(dpos, u + STEP_U);
	vec_sub(dpos, dpos, center);
	vec_scale(dpos, dpos, 1.0/STEP_U);

	eVector3 rnd = {0, 0, 1};
	eVector3 comp_0, comp_1;
	vec_cross(comp_0, dpos, rnd);
	vec_cross(comp_1, comp_0, dpos);
	vec_normalize(comp_0, comp_0);
	vec_normalize(comp_1, comp_1);
	vec_mac(vec, center, comp_0, sin(v) * rad);
	vec_mac(vec, vec, comp_1, cos(v) * rad);
}

struct VertexBuffer *genTorus(void)
{
	VB_Begin(5);
	

	float u, v;
	for (u=0; u<1.0; u += STEP_U)
		for (v = 0; v < 1.0; v += STEP_V)
		{
			
			float vertex[] = {
				0, 0, 0, u, v,
				0, 0, 0, u + STEP_U, v + STEP_V,
				0, 0, 0, u + STEP_U, v,

				0, 0, 0, u, v,
				0, 0, 0, u, v + STEP_V,
				0, 0, 0, u + STEP_U, v + STEP_V
			};
			
			int i;
			for (i=0; i<6; ++i)
			{
				vertex[i * 5 + 3] *= 40.0;
			}
			
			genTorusFnc(&vertex[0 * 5], u, v);
			genTorusFnc(&vertex[1 * 5], u + STEP_U, v + STEP_V);
			genTorusFnc(&vertex[2 * 5], u + STEP_U, v);
			genTorusFnc(&vertex[3 * 5], u, v);
			genTorusFnc(&vertex[4 * 5], u, v + STEP_V);
			genTorusFnc(&vertex[5 * 5], u + STEP_U, v + STEP_V);
			
			VB_Put(vertex, sizeof(vertex) / 4);
		}
	
	return VB_End();
}

struct Shader *sh_ps_texture, *sh_vs_texture, *sh_ps_font, *sh_vs_font;

int main(int argc, char **argv)
{
	regs = ioremap(0xec800000ULL, 0x20000, 1);
	if (!regs)
		exit(1);
	rb = rb_primary = ioremap(RINGBUFFER_BASE, RINGBUFFER_SIZE, 0);
	
	tex_fb.ptr = r32(0x6110);
	tex_fb.pitch = r32(0x6120) / 32;
	tex_fb.width = r32(0x6134);
	tex_fb.height = r32(0x6138);
	
	if (tex_fb.height > 720)
		tex_fb.height = 720;
	if (tex_fb.width > 1280)
		tex_fb.width = 1280;
	
#if 0
	time_t t = time(0);
	while (t == time(0));
	t = time(0) + 10;
	int nr = 0;
	while (t > time(0))
	{
		memcpy(rb, rb + RINGBUFFER_SIZE / 2, RINGBUFFER_SIZE / 2);
		++nr;
	}
	
	printf("%d kB/s (%d)\n", nr * (RINGBUFFER_SIZE/1024)/2 / 10, nr);
	return 0;
#endif

	u32 rb_primary_phys = RB_Alloc();

	memset((void*)rb, 0xCC, RINGBUFFER_SIZE);

	RS_Init(tex_fb.width, tex_fb.height);
	
	x.vp_xres = tex_fb.width;
	x.vp_yres = tex_fb.height;

#define V(x, y, z, col, u, v) FLOAT(x), FLOAT(y), FLOAT(z), FLOAT(u + .5), FLOAT(v + .5) 

// col

#define COL_0 0x3C515D
#define COL_1 0x3D6868
#define COL_2 0x40957F
#define COL_3 0xA7C686
#define COL_4 0xFCEE8C
#define COL_5 COL_0
#define COL_6 COL_2
#define COL_7 COL_4

	u32 c01[] = {
			// top
		V(-.5,  .5, -.5, COL_2, -.5, .5), V(-.5, -.5, -.5, COL_0, -.5, -.5), V(.5, -.5, -.5, COL_4, .5, -.5), 
		V( .5,  .5, -.5, COL_6, .5, .5), V(-.5,  .5, -.5, COL_2, -.5, .5), V(.5, -.5, -.5, COL_4, .5, -.5), 

			// bottom
		V(.5, -.5,  .5, COL_5, .5, -.5), V(-.5, -.5,  .5, COL_1, -.5, -.5), V(-.5,  .5,  .5, COL_3, -.5, .5), 
		V(.5, -.5,  .5, COL_5, .5, -.5), V(-.5,  .5,  .5, COL_3, -.5, .5), V( .5,  .5,  .5, COL_7, .5, .5), 

			// left
		V(.5, -.5,  .5, COL_5, -.5, .5), V(.5, .5, -.5, COL_6, .5, -.5), V(.5, -.5, -.5, COL_4, -.5, -.5),
		V(.5,  .5,  .5, COL_7, .5, .5), V(.5, .5, -.5, COL_6, .5, -.5), V(.5, -.5,  .5, COL_5, -.5, .5),

			// right
		V(-.5, .5, -.5, COL_2, .5, -.5), V(-.5, -.5,  .5, COL_1, -.5, .5), V(-.5, -.5, -.5, COL_0, -.5, -.5), 
		V(-.5, .5, -.5, COL_2, .5, -.5), V(-.5,  .5,  .5, COL_3, .5, .5), V(-.5, -.5,  .5, COL_1, -.5, .5), 

			// front
		V(-.5, .5, -.5, COL_2, -.5, -.5), V(.5, .5, -.5, COL_6, .5, -.5), V(-.5, .5, .5, COL_3, -.5, .5),
		V(-.5, .5,  .5, COL_3, -.5, .5), V(.5, .5, -.5, COL_6, .5, -.5), V( .5, .5, .5, COL_7, .5, .5),

			// right
		V(-.5, -.5, -.5, COL_0, -.5, -.5), V(-.5, -.5, .5, COL_1, -.5, .5), V(.5, -.5, -.5, COL_4, .5, -.5), 
		V(-.5, -.5,  .5, COL_1, -.5, .5), V( .5, -.5, .5, COL_5, .5, .5), V(.5, -.5, -.5, COL_4, .5, -.5), 
	};
#undef V



	VB_Begin(5);
	VB_Put(c01, sizeof(c01)/4);
	struct VertexBuffer *vb = VB_End();

#define V(x,y,z,col) FLOAT(x), FLOAT(y), FLOAT(z), col
	
	u32 tri[] = {
		V(0,  -1, 0, 0x000000ff),
		V(-1,  1, 0, 0x0000ff00),
		V(+1,  1, 0, 0x00ffffff),
	};

	VB_Begin(4);
	VB_Put(tri, sizeof(tri)/4);
	struct VertexBuffer *vb_tri = VB_End();


		/* load shaders.. */
	sh_ps_texture = SH_Load("texture.psu");
	SH_InitShader(sh_ps_texture);

	sh_vs_texture = SH_Load("texture.vsu");
	static const struct VBF_Format vbf_POS_UV={2, 
		{
			{0, 57, SWIZZLE_XYZ1},   // position, float3
//			{10, 6},   // color, char4
			{5, 37, SWIZZLE_XY__},   // float2
		}
	};
	SH_VBF_ApplyVFetchPatches(sh_vs_texture, 0,&vbf_POS_UV);
	SH_InitShader(sh_vs_texture);

	struct Shader *sh_ps_color, *sh_vs_color;
	sh_ps_color = SH_Load("color.psu");
	SH_InitShader(sh_ps_color);

	sh_vs_color = SH_Load("color.vsu");
	static const struct VBF_Format vbf_POS_COL={2, 
		{
			{0, 57, SWIZZLE_XYZ1},   // position, float3
			{10, 6, SWIZZLE_XYZW},   // color, char4
		}
	};
	SH_VBF_ApplyVFetchPatches(sh_vs_color, 0, &vbf_POS_COL);
	SH_InitShader(sh_vs_color);

	u32 *tex_ptr = MEM_Alloc(&tex_test.ptr, 256*256*4, 0x1000);
		// loadbin_phys("texture.raw", 0x1000);
	
	printf("Generating texture...\n");
			/* let's generate some texture... */
		/* taken from http://freespace.virgin.net/hugo.elias/models/m_perlin.htm: */
	float Noise(int x, int y)
	{
		int n = x + y * 57;
		n = (n<<13) ^n;
		return ( 1.0 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
	}
	
	float SmoothNoise1(float x, float y)
	{
		float corners = ( Noise(x-1, y-1)+Noise(x+1, y-1)+Noise(x-1, y+1)+Noise(x+1, y+1) ) / 16;
		float sides   = ( Noise(x-1, y)  +Noise(x+1, y)  +Noise(x, y-1)  +Noise(x, y+1) ) /  8;
		float center  =  Noise(x, y) / 4;
		return corners + sides + center;
	}

	float Interpolate(float a, float b, float frac)
	{
		return a + (b - a) * frac;
	}

	float InterpolatedNoise_1(float x, float y)
	{
		int integer_X    = x;
		float fractional_X = x - integer_X;

		int integer_Y    = y;
		float fractional_Y = y - integer_Y;

		float v1 = SmoothNoise1(integer_X,     integer_Y);
		float v2 = SmoothNoise1(integer_X + 1, integer_Y);
		float v3 = SmoothNoise1(integer_X,     integer_Y + 1);
		float v4 = SmoothNoise1(integer_X + 1, integer_Y + 1);

		float i1 = Interpolate(v1 , v2 , fractional_X);
		float i2 = Interpolate(v3 , v4 , fractional_X);

		return Interpolate(i1 , i2 , fractional_Y);
	}

	float PerlinNoise_2D(float x, float y)
	{
		float total = 0;
		float p = 0.9;

		int n = 8;
		while (n--)
		{
			int frequency = 1<<n;
			float amplitude = pow(p, n);
			total += InterpolatedNoise_1(x * frequency, y * frequency) * amplitude;
		}
		return total;
	}
	
	int y, x;
	u32 *p = tex_ptr;
	for (y=0; y<256; ++y)
		for (x=0; x<256; ++x)
		{
			unsigned char a, r, g, b;
			a = 0xff;
			r = PerlinNoise_2D(x / 256.0, y / 256.0) * 100.0 + 128.0;
			g = r;
			b = r;
			*p++ = (a<<24)|(r<<16)|(g<<8)|b;
		}
	MEM_SyncToDevice(tex_ptr, 256*256*4);
	
	tex_test.ptr_mip = 0;
	
	printf("done.\n");
	
 	INIT_MasterInit(rb_primary_phys);
	INIT_EnableWriteback(RINGBUFFER_BASE + RPTR_WRITEBACK, 6);
	
	MEM_SyncFromDevice(rb + RPTR_WRITEBACK, 4);
	printf("writeback: %08x\n", *(volatile u32*)(rb + RPTR_WRITEBACK));
	
	write_reg(0x0774, RINGBUFFER_BASE + SCRATCH_WRITEBACK);
	write_reg(0x0770, 0x20033);

	write_reg(0x15e0, 0x1234567);
	
	global_init();

	float frame = 0;
	int frameidx = 0x11223344;

	int frames = 0;

	M_BuildPersp(&g_proj, 45.0 / 180.0 * M_PI, 640.0/480.0, 1, 200.0);
	
	int rate = 0;
	
	struct VertexBuffer *t = genTorus();
	
	InvalidateGpuCache(RINGBUFFER_BASE, RINGBUFFER_SIZE);
	for (;;)
	{
		if (matrix_top)
		{
			fprintf(stderr, "runaway glPushMatrix.\n");
			break;
		}
		
		InvalidateState();
		glLoadIdentity();
		
//		gluLookAt(0, 0, 7, 0, 0, 0, 0, 1, 0);
		
		glPushMatrix();
		SetShader(SHADER_TYPE_PIXEL, sh_ps_texture);
		SetShader(SHADER_TYPE_VERTEX, sh_vs_texture);
		SetTexture(0, &tex_test);
		glTranslate(0, 0, -10);
		glRotate(frame * 10.0, 1,1,1);
		
				/* draw cube. */
		glPushMatrix();
		glScale(-1, -1, -1);
		M_LoadCurrent();
		draw(vb);
		glPopMatrix();

		M_LoadCurrent();
		SetTexture(0, &tex_test);
		SetShader(SHADER_TYPE_PIXEL, sh_ps_texture);
		SetShader(SHADER_TYPE_VERTEX, sh_vs_texture);
		SetDepthControl(3);
		SetZWrite(1);

		if (frameidx & 2048) /* from time to time, display wireframe */
			SetFillMode(1, 1);
		
		draw(t);
		
		SetShader(SHADER_TYPE_PIXEL, sh_ps_color);
		SetShader(SHADER_TYPE_VERTEX, sh_vs_color);
		draw(vb_tri);
		
		glPopMatrix();

		RS_Resolve(&tex_fb);

		write_reg(0x15e0, frameidx);
		block_until_idle();
		
		RB_Kick();
//		printf("waiting for frameidx %08x\n", frameidx);
		do {
			MEM_SyncFromDevice(rb + SCRATCH_WRITEBACK, 4);
		} while (*(volatile u32*)(rb + SCRATCH_WRITEBACK) != frameidx); // usleep(1000);

		frame += 0.001;
		frameidx++;
		
		frames++;
		
		static time_t lt;
		if (time(0) != lt)
		{
			rate = frames;
			printf("%d fps (%lld Mtri/s)\n", frames, tris_drawn / 1000 / 1000);
			frames=0;
			tris_drawn = 0;
			lt = time(0);
		}
 		VB_Reclaim();
	}

	return 0;
}
