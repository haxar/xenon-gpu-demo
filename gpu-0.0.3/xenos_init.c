#include "xenos.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void INIT_ResetRingbuffer(void)
{
	w32(0x0704, r32(0x0704) | 0x80000000);
	w32(0x017c, 0);
	w32(0x0714, 0);
	w32(0x0704, r32(0x0704) &~0x80000000);
}

void INIT_SetupRingbuffer(u32 buffer_base, u32 size_in_l2qw)
{
	INIT_ResetRingbuffer();
	w32(0x0704, size_in_l2qw | 0x8020000);
	w32(0x0700, buffer_base);
	w32(0x0718, 0x10);
}

void INIT_LoadUcodes(const u32 *ucode0, const u32 *ucode1)
{
	int i;
	
	w32(0x117c, 0);
	usleep(100);
	
	for (i = 0; i < 0x120; ++i)
		w32(0x1180, ucode0[i]);

	w32(0x117c, 0);
	usleep(100);
	for (i = 0; i < 0x120; ++i)
		r32(0x1180);

	w32(0x07e0, 0);
	for (i = 0; i < 0x900; ++i)
		w32(0x07e8, ucode1[i]);

	w32(0x07e4, 0);
	for (i = 0; i < 0x900; ++i)
		if (r32(0x07e8) != ucode1[i])
			break;

	if (i != 0x900)
		printf("[WARN] ucode1 microcode verify error\n");	
	else
		printf("ucode1 ok\n");
}

void INIT_WaitReady(void)
{
	printf("[INIT_WaitReady]\n");
	while (r32(0x1740) & 0x80000000);
}

void INIT_WaitReady2(void)
{
	printf("[INIT_WaitReady2]\n");
	while (!(r32(0x1740) & 0x00040000));
}

void INIT_Init1(void)
{
	w32(0x01a8, 0);
	w32(0x0e6c, 0xC0F0000);
	w32(0x3400, 0x40401);
	usleep(1000);
	w32(0x3400, 0x40400);
	w32(0x3300, 0x3A22);
	w32(0x340c, 0x1003F1F);
	w32(0x00f4, 0x1E);
}

void INIT_Reset(void)
{
	INIT_WaitReady2();
//	INIT_WaitReady();
#if 0
	printf("waiting for reset.\n");
	do {
		w32(0x00f0, 0x8064); r32(0x00f0);
		w32(0x00f0, 0);
		w32(0x00f0, 0x11800); r32(0x00f0);
		w32(0x00f0, 0);
		usleep(1000);
	} while (r32(0x1740) & 0x80000000);
#endif

	INIT_Init1();
}

void INIT_Init0(u32 buffer_base, u32 size_in_l2qw)
{
	w32(0x07d8, 0x1000FFFF);
	usleep(2000);
	w32(0x00f0, 1);
	(void)r32(0x00f0);
	usleep(1000);
	w32(0x00f0, 0);
	usleep(1000);
	INIT_SetupRingbuffer(buffer_base, size_in_l2qw);
	INIT_WaitReady();

	if (!(r32(0x07d8) & 0x10000000))
		printf("[WARN] something wrong (1)\n");

	w32(0x07d8, 0xFFFF);
	usleep(1000);

	w32(0x3214, 7);
	w32(0x3294, 1);
	w32(0x3408, 0x800);
	
	INIT_WaitReady();
	
	if (r32(0x0714))
		printf("[WARN] something wrong (2)\n");
	
	if (r32(0x0710))
		printf("[WARN] something wrong (3)\n");

	w32(0x07ec, 0x1A);
}

void INIT_Setup(u32 buffer_base, u32 buffer_size, const u32 *ucode0, const u32 *ucode1)
{
	INIT_WaitReady();

	w32(0x07d8, 0x1000FFFF);
	
	INIT_SetupRingbuffer(buffer_base, buffer_size);
	INIT_LoadUcodes(ucode0, ucode1);
	INIT_WaitReady();
	
	w32(0x07d8, 0xFFFF);
	w32(0x07d0, 0xFFFF);
	w32(0x07f0, 0);
	w32(0x0774, 0);
	w32(0x0770, 0);
	w32(0x3214, 7);
	w32(0x3294, 1);
	w32(0x3408, 0x800);
	INIT_Init0(buffer_base, buffer_size);
	INIT_WaitReady();
}

u32 ucode0[0x120], ucode1[0x900];

void INIT_MasterInit(u32 buffer_base)
{
	if ((r32(0x0e6c) & 0xF00) != 0xF00)
		printf("something wrong (3)\n");

	printf("0x0e6c: %08x\n", r32(0x0e6c));

	FILE *f = fopen("ucode0.bin", "rb");
	if (!f)
	{
		perror("ucode0.bin");
		exit(1);
	}
	fread(ucode0, 0x120*4, 1, f);
	fclose(f);

	f = fopen("ucode1.bin", "rb");
	if (!f)
	{
		perror("ucode1.bin");
		exit(1);
	}
	fread(ucode1, 0x900*4, 1, f);
	fclose(f);

	INIT_Setup(buffer_base, 0xC, ucode0, ucode1);

	w32(0x07d4, 0);
	w32(0x07d4, 1);

	w32(0x2054, 0x1E);
	w32(0x2154, 0x1E);
	
	w32(0x3c10, 0xD);
	
	w32(0x3c40, 0x17);
	w32(0x3c48, 0);
	while (r32(0x3c4c) & 0x80000000);

	w32(0x3c40, 0x1017);
	w32(0x3c48, 0);
	while (r32(0x3c4c) & 0x80000000);

	w32(0x87e4, 0x17);
}

void INIT_EnableWriteback(u32 addr, int blocksize)
{
	u32 v = r32(0x0704);

	v &= ~0x8003F00;
	w32(0x0704, v);
	
	w32(0x070c, addr | 2);
	w32(0x0704, v | (blocksize << 8));
}
