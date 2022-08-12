#ifndef __xenos_h
#define __xenos_h

#include <string.h>

typedef unsigned int u32;
typedef unsigned short u16;

#include "xenos_struct.h"
#include "xenos_consts.h"
#include "engine.h"

extern unsigned int r32(int offset);
extern void w32(int offset, unsigned int val);

extern u32 rb_secondary_base;
extern volatile void *rb, *rb_primary, *rb_secondary;
extern int rb_primary_wptr, rb_secondary_wptr;
extern int rb_secondary_boundary;

extern volatile unsigned int *regs;

extern void INIT_MasterInit(u32 buffer_base);
extern void INIT_EnableWriteback(u32 addr, int blocksize);
extern void global_init(void);

#define SHADER_TYPE_PIXEL 1
#define SHADER_TYPE_VERTEX 0

extern void LoadShader(int base, int type, int size);

	/* Matrix functions */
extern void M_LoadMatrix44(int base, eMatrix44 *matrix);
extern void M_LoadMatrix43(int base, eMatrix43 *matrix);
extern void M_LoadCurrent(void);
extern void M_BuildPersp(eMatrix44 *m, float fovy, float aspect, float f, float n);
extern void M_Dump(const char *name, eMatrix44 *m);

extern eMatrix44 g_ident;
extern eMatrix44 g_proj;

	/* Resolve functions */
void RS_Resolve(struct Texture *target);
void RS_Init(int xres, int yres);

	/* Shaders */

extern struct Shader *sh_ps_texture, *sh_vs_texture, *sh_ps_font, *sh_vs_font;

struct Shader
{
	void *shader;
	u32 shader_phys, shader_phys_size, program_control, context_misc;
};

extern struct Shader *SH_Load(const char *filename);
extern void SH_Upload_Constants(struct Shader *s);
extern void SH_InitShader(struct Shader *sh);

#define SWIZZLE_XYZW 0x688
#define SWIZZLE_XYZ1 0xA88
#define SWIZZLE_XY__ 0xFC8

	/* each vertex buffer element fills FOUR FLOAT components.
	   the 'usage' specifies which of them (position, color, texuv, ..)
	   the 'fmt' specified in which form they lie in memory. if you 
	     specify float3, the remaining component will be filled up with
	     the 0 or 1, according to the swizzling.
	*/
struct VBF_Element
{
	int usage; // like in shaders: 0 for position, 10 for color, ...
	int fmt; // 6 for char4, 57 for float3, 38 for float4, 37 for float2, ...
	int swizzle; // wwwzzzyyyxxx with {x,y,z,w,0,1,_}. so 011010001000 == 0x688 is default.
};

struct VBF_Format
{
	int num;
	struct VBF_Element e[];
};

extern int SH_VBF_CalcSize(const struct VBF_Element *fmt);
extern int SH_VBF_CalcStride(const struct VBF_Format *fmt);
extern void SH_VBF_ApplyVFetchPatches(struct Shader *sh, int index, const struct VBF_Format *fmt);

	/* State */
extern int stat_alu_uploaded;
extern struct XenosState x;
extern void InitControl(void);
extern void SetDepthControl(int z_func);
extern void SetZWrite(int zw);
extern void SetFillMode(int front, int back);
extern void SetBlendControl(int col_src, int col_op, int col_dst, int alpha_src, int alpha_op, int alpha_dst);
extern void InvalidateState(void);
extern void SetShader(int type, struct Shader *sh);
extern void SetState(void);
extern void DirtyAluConstant(int base, int len);
extern void DirtyFetch(int base, int len);
extern void SetTexture(int index, struct Texture *tex);

	/* VB */
extern struct VertexBuffer *VB_Alloc(int size);
extern struct VertexBuffer *VB_PoolAlloc(int size);
extern void VB_PoolAdd(struct VertexBuffer *vb);
extern void VB_Reclaim(void);

extern void VB_Begin(int pitch);
extern void VB_Put(void *data, int len);
extern struct VertexBuffer *VB_End(void);

	/* Ringbuffer */
void RB_KickSegment(int base, int len);
void RB_Kick(void);
void RB_MayKick(void);
u32 RB_Alloc(void);
void RB_CommitPrimary(void);

	/* Memory */
void MEM_SyncToDevice(volatile void *data, int len);
void MEM_SyncFromDevice(volatile void *data, int len);
void *MEM_Alloc(u32 *phys, int size, int align);

	/* Func */
void InvalidateGpuCache(int base, int size);
void InvalidateGpuCacheAll(int base, int size);
void SetSurfaceClip(int offset_x, int offset_y, int sc_left, int sc_top, int sc_right, int sc_bottom);
void SetBin(u32 mask_low, u32 select_low, u32 mask_hi, u32 select_hi);
void ControlPacket(u32 depth_control, u32 blend_control_0, u32 color_control, u32 hi_control, u32 clip_control, u32 mode_control, u32 vte_control, u32 edram_mode_control, u32 blend_control_1, u32 blend_control_2, u32 blend_control_3);
void WaitUntilIdle(u32 what);
void DrawNonIndexed(int num_points);
void SetIndexOffset(int offset);
void VERTEX_FETCH(u32 *dst, u32 base, int len);
void TEXTURE_FETCH(u32 *dst, u32 base, int width, int height, int pitch, int tiled, int format, u32 base_mip, int anisop);
void LoadShader(int base, int type, int size);
void check_irqs(void);
void align(void);
void block_until_idle(void);
void step(u32 x);
void InvalidateGpuCache_Primary(int base, int size);



#include "xenos_inline.h"

	/* junk */
extern void _stuff(void);

#endif
