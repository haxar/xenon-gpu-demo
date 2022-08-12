#include "xenos.h"

void GINIT_Init0(void)
{
	rput32(0xc0003b00);
		rput32(0x00000300);
	
	rput32(0xc0192b00);
		rput32(0x00000000); rput32(0x00000018); 
		rput32(0x00001003); rput32(0x00001200); rput32(0xc4000000); rput32(0x00001004); 
		rput32(0x00001200); rput32(0xc2000000); rput32(0x00001005); rput32(0x10061200); 
		rput32(0x22000000); rput32(0xc8000000); rput32(0x00000000); rput32(0x02000000); 
		rput32(0xc800c000); rput32(0x00000000); rput32(0xc2000000); rput32(0xc888c03e); 
		rput32(0x00000000); rput32(0xc2010100); rput32(0xc8000000); rput32(0x00000000); 
		rput32(0x02000000); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000);
	rput32(0xc00a2b00);
		rput32(0x00000001); rput32(0x00000009); 
		
		rput32(0x00000000); rput32(0x1001c400); rput32(0x22000000); rput32(0xc80f8000); 
		rput32(0x00000000); rput32(0xc2000000); rput32(0x00000000); rput32(0x00000000); 
		rput32(0x00000000);

	rput32(0x00012180);
		rput32(0x1000000e); rput32(0x00000000);
	rput32(0x00022100);
		rput32(0x0000ffff); rput32(0x00000000); rput32(0x00000000);
	rput32(0x00022204); 
		rput32(0x00010000); rput32(0x00010000); rput32(0x00000300);
	rput32(0x00002312); 
		rput32(0x0000ffff);
	rput32(0x0000200d); 
		rput32(0x00000000);
	rput32(0x00002200); 
		rput32(0x00000000);
	rput32(0x00002203); 
		rput32(0x00000000);
	rput32(0x00002208); 
		rput32(0x00000004);
	rput32(0x00002104); 
		rput32(0x00000000);
	rput32(0x00002280); 
		rput32(0x00080008);
	rput32(0x00002302); 
		rput32(0x00000004);
	SetSurfaceClip(0, 0, 0, 0, 16, 16);
}

void GINIT_Init1(int arg)
{
	rput32(0x000005c8); 
		rput32(0x00020000);
	rput32(0x00078d00); 
		rput32(arg | 1); rput32(arg | 1); rput32(arg | 1); rput32(arg | 1); 
		rput32(arg | 1); rput32(arg | 1); rput32(arg | 1); rput32(arg | 1);
	rput32(0x00000d00); 
		rput32(arg);
}

void GINIT_Init2(void)
{
	int i;
	for (i=0; i<24; ++i)
	{
		rput32(0xc0003600);
			rput32(0x00010081);
	}
}

void GINIT_Init3(void)
{
	rput32(0x000005c8); 
		rput32(0x00020000);
	rput32(0x00000d04); 
		rput32(0x00000000);
}

void GINIT_Init4(void)
{
	rput32(0x00000d02); 
		rput32(0x00010800);
	rput32(0x00030a02); 
		rput32(0xc0100000); rput32(0x07f00000); rput32(0xc0000000); rput32(0x00100000);

	GINIT_Init3();
}

void GINIT_Init5(void)
{
	rput32(0x00000d01); 
		rput32(0x04000000);
	rput32(0xc0022100); 
		rput32(0x00000081); rput32(0xffffffff); rput32(0x80010000);
	rput32(0xc0022100); 
		rput32(0x00000082); rput32(0xffffffff); rput32(0x00000000);
	rput32(0x00000e42); 
		rput32(0x00001f60);
	rput32(0x00000c85); 
		rput32(0x00000003);
	rput32(0x0000057c); 
		rput32(0x0badf00d);
	rput32(0x0000057b); 
		rput32(0x00000000);
}

void GINIT_Init6(void)
{
	SetSurfaceClip(0, 0, 0, 0, 1024, 720);
	rput32(0x0002857e); 
		rput32(0x00010017); rput32(0x00000000); rput32(0x03ff02cf);
	rput32(0x0002857e); 
		rput32(0x00010017); rput32(0x00000004); rput32(0x03ff02cf);
}

void GINIT_Init7(void)
{
	rput32(0x000005c8); 
		rput32(0x00020000);
	rput32(0x00000f01); 
		rput32(0x0000200e);
}

void GINIT_Init8(void)
{
	SetSurfaceClip(0, 0, 0, 0, 1024, 720);
}

void GINIT_Init9(void)
{
	int i;
	rput32(0x0000057e);
		rput32(0x00010019);
		
	GINIT_Init0();
	
	for (i = 0x10; i <= 0x70; ++i)
		GINIT_Init1(0x00000000 | (i << 12) | ((0x80 - i) << 4));

	GINIT_Init2();
	rput32(0x0000057e); 
		rput32(0x0001001a);

	GINIT_Init8();
}

void GINIT_Init10(void)
{
	SetSurfaceClip(0, 0, 0, 0, 1024, 720);

	rput32(0x0000057e); 
		rput32(0x00010019);
	rput32(0xc0003b00); 
		rput32(0x00000300);

	GINIT_Init7();

	GINIT_Init9();
}

void global_init(void)
{
	rput32(0xc0114800); 
		rput32(0x000003ff); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000); 
		rput32(0x00000080); rput32(0x00000100); rput32(0x00000180); rput32(0x00000200); 
		rput32(0x00000280); rput32(0x00000300); rput32(0x00000380); rput32(0x00010800); 
		rput32(0x00000007); rput32(0x00000000); rput32(0x00000000); rput32(0x00000000); 
		rput32(0x00000000); rput32(0x00000000);

	GINIT_Init4();

	GINIT_Init5();
	GINIT_Init6();
	GINIT_Init10();
}
