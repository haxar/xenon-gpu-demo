static inline int FLOAT(float f)
{
	union {
		float f;
		u32 d;
	} u = {f};
	
	return u.d;
}

static inline void rput32(u32 d)
{
	*(volatile u32*)(rb_secondary + rb_secondary_wptr++ * 4) = d;
}

static inline void rput(void *base, int len)
{
	memcpy(((void*)rb_secondary) + rb_secondary_wptr * 4, base, len * 4);
	rb_secondary_wptr += len;
}

static inline void rput32p(u32 d)
{
	*(volatile u32*)(rb_primary + rb_primary_wptr++ * 4) = d;
	
	if (rb_primary_wptr == RINGBUFFER_PRIMARY_SIZE)
		rb_primary_wptr = 0;
}

extern void align(void);

static inline void rputf(float d)
{
	rput32(FLOAT(d));
}



/* CP command packets */
#define RADEON_CP_PACKET0               0x00000000
#       define RADEON_ONE_REG_WR                (1 << 15)
#define RADEON_CP_PACKET1               0x40000000
#define RADEON_CP_PACKET2               0x80000000
#define RADEON_CP_PACKET3               0xC0000000
             

#define CP_PACKET0( reg, n )                                            \
        (RADEON_CP_PACKET0 | ((n) << 16) | ((reg) >> 2))
#define CP_PACKET0_TABLE( reg, n )                                      \
        (RADEON_CP_PACKET0 | RADEON_ONE_REG_WR | ((n) << 16) | ((reg) >> 2))
#define CP_PACKET1( reg0, reg1 )                                        \
        (RADEON_CP_PACKET1 | (((reg1) >> 2) << 15) | ((reg0) >> 2))
#define CP_PACKET2()                                                    \
        (RADEON_CP_PACKET2)
#define CP_PACKET3( pkt, n )                                            \
        (RADEON_CP_PACKET3 | (pkt) | ((n) << 16))


static inline void write_reg(u32 reg, u32 val)
{
	rput32(CP_PACKET0(reg, 0));
	rput32(val);
}

static inline u32 SurfaceInfo(int surface_pitch, int msaa_samples, int hi_zpitch)
{
	return surface_pitch | (msaa_samples << 16) | (hi_zpitch << 18);
}

static inline u32 xy32(int x, int y)
{
	return x | (y << 16);
}
