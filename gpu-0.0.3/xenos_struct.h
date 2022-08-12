struct XenosState
{
	float alu_constants[256 * 4 * 2];
	u32 fetch_constants[96 * 2];
	
	u32 alu_dirty; /* 16 * 4 constants per bit */
	u32 fetch_dirty; /* 3 * 2 per bit */
	
	float clipplane[6*4];
	u32 integer_constants[10*4];
	u32 controlpacket[9];
	
	struct Shader *vs, *ps;

#define DIRTY_ALU      0x0001
#define DIRTY_FETCH    0x0002
#define DIRTY_CLIP     0x0004
#define DRITY_INTEGER  0x0008
#define DIRTY_CONTROL  0x0010
#define DIRTY_SHADER   0x0020
	int dirty;
	
	int vp_xres, vp_yres;
};

struct Texture
{
	int width, height, pitch, tiled, format;
	u32 ptr, ptr_mip;
};


#define FORMAT_8888 6

#define FORMAT_ARGB 0x80
#define FORMAT_BGRA 0x00


struct VertexBuffer
{
	u32 phys_base;
	int vertices;
	int size, space; /* in DWORDs */
	void *base;
	
	struct VertexBuffer *next;
};

