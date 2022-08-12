/*
 * extract_ucode - extracts required GPU microcodes from flash.
 * must be run on a 360 (required /dev/mem access, so run as root.)
 * compile and link against libmspack.
 */

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

	/* libmspack */
#include <mspack/mspack.h>
#include <mspack/system.h>
#include <mspack/lzx.h>

#include "hmac_sha1.h"

#include <byteswap.h>

#define STATUS  1
#define COMMAND 2
#define ADDRESS 3
#define DATA    4

volatile unsigned int *flash;

void writereg(int reg, int value)
{
	flash[reg] = bswap_32(value);
}

unsigned int readreg(int reg)
{
	return bswap_32(flash[reg]);
}

void read_flash_block(int sector, void *data)
{
	int status;
	writereg(STATUS, readreg(STATUS));
	writereg(ADDRESS, sector * 512);
	writereg(COMMAND, 2);

	while ((status = readreg(STATUS))&1);

	if (status != 0x300)
		fprintf(stderr, "error reading %08x: %08x\n", sector, status);

	writereg(ADDRESS, 0);
	int i;

	for (i=0; i<128; ++i)
	{
		writereg(COMMAND, 0);
		*(int*)data = bswap_32(readreg(DATA));
		data += 4;
	}
}

void read_flash(void *data, int offset, int len)
{
	while (len)
	{
		int aligned_offset = offset & ~0x1FF;
		int misalign = offset & 0x1FF;
		unsigned char block[0x200];
		int rd = 0x200;
		rd -= misalign;
		if (rd > len)
			rd = len;
		read_flash_block(aligned_offset / 0x200, block);
		memcpy(data, block + misalign, rd);
		offset += rd;
		data += rd;
		len -= rd;
	}
}

unsigned int get32(void *d)
{
	unsigned char *b = d;
	return (b[0] << 24) | (b[1] << 16) | (b[2] <<8) | b[3];
}

unsigned short get16(void *d)
{
	unsigned char *b = d;
	return (b[0] <<8) | b[1];
}

unsigned int read_32(int offset)
{
	unsigned char b[4];
	read_flash(b, offset, 4);
	return get32(b);
}

void hmac_sha(void *secret, void *data, void *res)
{
	unsigned char out[20];
	HMAC_SHA1_CTX ctx;
	HMAC_SHA1_Init(&ctx);
	HMAC_SHA1_UpdateKey(&ctx, secret, 0x10);
	HMAC_SHA1_EndKey(&ctx);
	HMAC_SHA1_StartMessage(&ctx);
	HMAC_SHA1_UpdateMessage(&ctx, data, 0x10);
	HMAC_SHA1_EndMessage(out, &ctx);
	HMAC_SHA1_Done(&ctx);
	memcpy(res, out, 0x10);
}

	/* from http://b-con.us/files/code/rc4.c, modified to meet my coding style.  */
void rc4_init(unsigned char *state, unsigned char *key, int len)
{
	int i, j=0, t; 

	for (i=0; i < 256; ++i)
		state[i] = i; 

	for (i=0; i < 256; ++i) {
		j = (j + state[i] + key[i % len]) % 256; 
		t = state[i]; 
		state[i] = state[j]; 
		state[j] = t; 
	}	
}

void rc4_crypt(unsigned char *state, unsigned char *data, int len)
{  
	int i=0,j=0,x,t; 

	for (x=0; x < len; ++x)  {
		i = (i + 1) % 256;
		j = (j + state[i]) % 256;
		t = state[i];
		state[i] = state[j];
		state[j] = t;
		*data++ ^= state[(state[i] + state[j]) % 256];
	}
}  

void hexdump(unsigned char *data, int len)
{
	while (len--)
		printf(" %02x", *data++);
	printf("\n");
}

struct mspack_mem_p {
	void *start, *end;
	int remaining;
};

static struct mspack_file *msp_open(struct mspack_system *this,
				    char *filename, int mode)
{
  struct mspack_mem_p *fh;

  if ((fh = malloc(sizeof(struct mspack_mem_p))))
    return (struct mspack_file *) fh;
  return NULL;
}

static void msp_close(struct mspack_file *file) {
  struct mspack_mem_p *this = (struct mspack_mem_p *) file;
  if (this) {
    free(this);
  }
}

static int msp_read(struct mspack_file *file, void *buffer, int bytes) {
  struct mspack_mem_p *this = (struct mspack_mem_p *) file;
  if (this) {
  	int max = this->remaining;
  	if (!max)
  	{
  		if (this->start < this->end)
  		{
				int compressed_size = get16(this->start); this->start += 2;
				// int uncompressed_size = get16(this->start); 
				this->start += 2;
				max = this->remaining = compressed_size;
  		} else
  			return 0;
  	}
  	if (bytes > max)
  		bytes = max;
  	memcpy(buffer, this->start, bytes);
  	this->start += bytes;
  	this->remaining -= bytes;
  	return bytes;
  }
  return -1;
}

static int msp_write(struct mspack_file *file, void *buffer, int bytes) {
  struct mspack_mem_p *this = (struct mspack_mem_p *) file;
  if (this) {
  	memcpy(this->end, buffer, bytes);
  	this->end += bytes;
  	return bytes;
  }
  return -1;
}

static int msp_seek(struct mspack_file *file, off_t offset, int mode) {
  return -1;
}

static off_t msp_tell(struct mspack_file *file) {
	return 0;
}

static void msp_msg(struct mspack_file *file, char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fputc((int) '\n', stderr);
  fflush(stderr);
}

static void *msp_alloc(struct mspack_system *this, size_t bytes) {
#ifdef DEBUG
  /* make uninitialised data obvious */
  char *buf = malloc(bytes + 8);
  if (buf) memset(buf, 0xDC, bytes);
  *((size_t *)buf) = bytes;
  return &buf[8];
#else
  return malloc(bytes);
#endif
}

static void msp_free(void *buffer) {
#ifdef DEBUG
  char *buf = buffer;
  size_t bytes;
  if (buf) {
    buf -= 8;
    bytes = *((size_t *)buf);
    /* make freed data obvious */
    memset(buf, 0xED, bytes);
    free(buf);
  }
#else
  free(buffer);
#endif
}

static void msp_copy(void *src, void *dest, size_t bytes) {
  memcpy(dest, src, bytes);
}

static struct mspack_system msp_mem = {
  &msp_open, &msp_close, &msp_read,  &msp_write, &msp_seek,
  &msp_tell, &msp_msg, &msp_alloc, &msp_free, &msp_copy, NULL
};

extern volatile void * ioremap(unsigned long long physaddr, unsigned size, int sync);
extern int iounmap(volatile void *start, size_t length);

int main(int argc, char **argv)
{
	void *data_1bl = (void*)ioremap(0x20000000000ULL, 0x8000, 0);
	if (!data_1bl)
		exit(1);
	int offset_1bl_key = get32(data_1bl + 0xFC) + 0x148;

	unsigned char key_1bl[16];
	memcpy(key_1bl, data_1bl + offset_1bl_key, 0x10);

	flash = ioremap(0xea00c000, 0x1000, 1);

	int offset_2bl = read_32(0x8);

	int build_2bl = read_32(offset_2bl) & 0xFFFF;
	printf("2bl build: %d\n", build_2bl);

	if (build_2bl >= 1920)
	{
		printf("sorry, you are out of luck. your 4BL encryption is unsupported yet.\n");
		printf("BUT: since you are here already, we can fix this. Please contact the author.\n");
		exit(1);
	}

	unsigned char key_2bl[0x10];
	read_flash(key_2bl, offset_2bl + 0x10, 0x10);
	hmac_sha(key_1bl, key_2bl, key_2bl);
	printf("2bl key: ");
	hexdump(key_2bl, 0x10);

	int offset_4bl = read_32(offset_2bl + 0xc) + offset_2bl;
	unsigned char key_4bl[0x10];
	read_flash(key_4bl, offset_4bl + 0x10, 0x10);
	hmac_sha(key_2bl, key_4bl, key_4bl);
	printf("4bl key: ");
	hexdump(key_4bl, 0x10);

	int offset_5bl = read_32(offset_4bl + 0xc) + offset_4bl;
	unsigned char key_5bl[0x10];
	read_flash(key_5bl, offset_5bl + 0x10, 0x10);
	hmac_sha(key_4bl, key_5bl, key_5bl);
	printf("5bl key: ");
	hexdump(key_5bl, 0x10);

	unsigned char rc4[0x100];
	rc4_init(rc4, key_5bl, 0x10);
	int len_5bl = read_32(offset_5bl + 0xC);
	unsigned char *data_5bl = malloc(len_5bl);
	read_flash(data_5bl, offset_5bl, len_5bl);
	rc4_crypt(rc4, data_5bl + 0x20, len_5bl - 0x20);

	void *p = data_5bl + 0x30;

	int output_length = get32(data_5bl + 0x28);

	struct mspack_system *sys = &msp_mem;
	struct mspack_file *input, *output;
	input = sys->open(sys, 0, 0);
	output = sys->open(sys, 0, 0);
	struct mspack_mem_p *o = (struct mspack_mem_p *) output;
	struct mspack_mem_p *i = (struct mspack_mem_p *) input;

	o->start = o->end = malloc(output_length);
	i->start = p;
	i->remaining = 0;
	i->end = data_5bl + len_5bl;

	struct lzxd_stream *lzxd = lzxd_init(sys, input, output, 17, 0, 0x10000, output_length);
	if (!lzxd)
		printf("lzxd_init failed\n");

	while (o->end - o->start != output_length)
	{
		int r = lzxd_decompress(lzxd, output_length - (o->end - o->start));
		if (r && r != 3)
		{
			printf("decompress failed (err=%d).\n", r);
			return 1;
		}
		if (r)
			break;
	}
	lzxd_free(lzxd);
	if (o->end - o->start != output_length)
	{
		printf("ran out of data\n");
		return 1;
	}
	free(data_5bl);

	void *base_image = o->start;
	
	int offset_6bl = read_32(0x64);
	int build_6bl = read_32(offset_6bl) & 0xFFFF;
	
	if (build_6bl < 4532)
	{
		offset_6bl += 0x10000;
		build_6bl = read_32(offset_6bl) & 0xFFFF;
		if (build_6bl < 4532)
		{
			printf("sorry, couldn't found the proper system update.\n");
			exit (1);
		}
	}
	
	printf("6bl build: %d\n", build_6bl);

	unsigned char hdr_6bl[0x340];
	read_flash(hdr_6bl, offset_6bl, 0x340);
	unsigned char *key_6bl = hdr_6bl + 0x20;
	hmac_sha(key_1bl, key_6bl, key_6bl);
	printf("6bl key: ");
	hexdump(key_6bl, 0x10);
	rc4_init(rc4, key_6bl, 0x10);
	rc4_crypt(rc4, hdr_6bl + 0x30, 0x310);
	int len_6bl = read_32(offset_6bl + 0xC);
	int offset_7bl = len_6bl + offset_6bl;

	unsigned char key_7bl[0x10];
	read_flash(key_7bl, offset_7bl + 0x10, 0x10);
	hmac_sha(hdr_6bl + 0x330, key_7bl, key_7bl);
	printf("7bl key: ");
	hexdump(key_7bl, 0x10);

	int len_7bl = read_32(offset_7bl + 0xC);
	void *data_7bl = malloc(len_7bl);
	int remaining = ( - len_6bl) & 0xFFFF;
	printf("remaining: %08x, %08x\n", remaining, len_6bl);
	read_flash(data_7bl, offset_7bl, remaining);
	void *d = data_7bl + remaining;

	void *chunks = hdr_6bl + 0x30;
	int nr_chunks = get16(chunks); chunks += 2;
	printf("%d additional chunks..\n", nr_chunks);

	remaining = len_7bl - remaining;

	while (nr_chunks--)
	{
		int offset_chunk = get16(chunks) * 0x4000; chunks += 2;
		int rd = remaining;
		if (rd > 0x4000)
			rd = 0x4000;

		read_flash(d, offset_chunk, rd);
		d += rd;
		remaining -= rd;
	}

	rc4_init(rc4, key_7bl, 0x10);
	rc4_crypt(rc4, data_7bl + 0x20, len_7bl - 0x20);

	int new_length = get32(data_7bl + 0x38);
	void *new_image = malloc(new_length);
	memset(new_image, 0, new_length);
	memcpy(new_image, base_image, output_length);
	free(base_image);

	d = data_7bl + 0x50;
	while (d < data_7bl + len_7bl)
	{
		int old_address = get32(d); d += 4;
		int new_address = get32(d); d += 4;
		int uncompressed_size = get16(d); d += 2;
		int compressed_size = get16(d); d += 2;

		printf("%08x, %08x, %04x, %04x %08x\n", old_address, new_address, uncompressed_size, compressed_size, d - data_7bl);
		if (compressed_size == 1) /* copy */
		{
			memmove(new_image + new_address, new_image + old_address, uncompressed_size);
		} else if (compressed_size == 0) /* zero */
		{
			memset(new_image + new_address, 0, uncompressed_size);
		} else
		{
			i->start = d;
			i->end = d + compressed_size;
			i->remaining = compressed_size;
			o->start = o->end = new_image + new_address;
			struct lzxd_stream *lzxd = lzxd_init(sys, input, output, 15, 0, 0x10000, uncompressed_size);
			if (!lzxd)
				printf("lzxd_init failed\n");
			memset(lzxd->window, 0, lzxd->window_size);
			memcpy(lzxd->window + lzxd->window_size - uncompressed_size, new_image + old_address, uncompressed_size);
			while (o->end != (o->start + uncompressed_size))
			{
				int r = lzxd_decompress(lzxd, uncompressed_size - (o->end - o->start));
				if (r && r != 3)
				{
					printf("decompress failed (err=%d).\n", r);
					return 1;
				}
				if (r)
					break;
			}
			d += compressed_size;
			lzxd_free(lzxd);
		}
	}
	free(data_7bl);

	unsigned long xorsum(void *data, int len)
	{
		int i;
		unsigned int res=0;
		for (i=0; i<len; i+=4)
			res ^= *(unsigned int*)(data + i);
		return res;
	}
#define HASHLEN 64
#define HASH_UCODE0 0x006794df
#define HASH_UCODE1 0xc1c0f602

	int ptr;
	int ok = 0;
	unsigned int res = 0;
	for (ptr = 0; ptr < new_length - HASHLEN; ptr += 4)
	{
		if (res == HASH_UCODE0)
		{
			ok |= 1;
			printf("found ucode0 at %08x\n", ptr);
			FILE *f = fopen("ucode0.bin", "wb");
			fwrite(new_image + ptr - HASHLEN, 0x120 * 4, 1, f);
			fclose(f);
		}

		if (res == HASH_UCODE1)
		{
			ok |= 2;
			printf("found ucode1 at %08x\n", ptr);
			FILE *f = fopen("ucode1.bin", "wb");
			fwrite(new_image + ptr - HASHLEN, 0x900 * 4, 1, f);
			fclose(f);
		}

		res ^= *(unsigned int*)(new_image + ptr);
		if (ptr >= HASHLEN)
			res ^= *(unsigned int*)(new_image + ptr - HASHLEN);
	}
	
	if (ok != 3)
		printf("sorry, ucode couldn't be found\n");

	free(new_image);

	return 0;
}
