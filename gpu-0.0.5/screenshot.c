#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <png.h>
#include <byteswap.h>

#define r32(o) regs[(o)/4]
volatile void * ioremap(unsigned long physaddr, unsigned size, int sync);

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <file.png>\n", *argv);
		return 1;
	}

	volatile unsigned int *regs = ioremap(0xec800000ULL, 0x20000, 1);
	if (!regs)
	{
		fprintf(stderr, "ioremap failed - %m");
		return 1;
	}

	int ptr = r32(0x6110);
	int pitch = r32(0x6120) * 4;
	int width = r32(0x6134);
	int height = r32(0x6138);

	volatile unsigned int *screen = (void*)ioremap(ptr, height * pitch, 0);

	unsigned int screen2[width * height];
	png_bytep row_pointers[height];
	
	int y, x;
	for (y=0; y<height; ++y)
	{
		for (x=0; x<width; ++x)
		{
			unsigned int base = ((((y & ~31)*width) + (x & ~31)*32 ) +
			 (((x&3) + ((y&1)<<2) + ((x&28)<<1) + ((y&30)<<5)) ^ ((y&8)<<2)));
			screen2[y * width + x] =  0xFF | bswap_32(screen[base] >> 8);
		}
		row_pointers[y] = screen2 + y * width;
	}
	
	FILE *outfp = fopen(argv[1], "wb");
	if (!outfp)
	{
		perror(argv[1]);
		return 2;
	}
	png_structp png_ptr_w = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop info_ptr_w = png_create_info_struct(png_ptr_w);
	if (setjmp(png_jmpbuf(png_ptr_w)))
	{
	  png_destroy_write_struct(&png_ptr_w, &info_ptr_w);
	  fclose(outfp);
	  return 1;
  }
	png_init_io(png_ptr_w, outfp);
	png_set_IHDR(png_ptr_w, info_ptr_w, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_set_rows(png_ptr_w, info_ptr_w, row_pointers);
	png_write_png(png_ptr_w, info_ptr_w, PNG_TRANSFORM_IDENTITY, 0);
	png_write_end(png_ptr_w, info_ptr_w);
	png_destroy_write_struct(&png_ptr_w, &info_ptr_w);

	fclose(outfp);
	
	return 0;
}
