Xenos 3D Library, Version 0.0.5

This code resembles a library to use the 3D functionality of the 'xenos'
chip in the Xbox 360.

It was written from scratch, and contains no microsoft code. All information
were reverse engineered from either code or ringbuffer dumps. 
No chip documentation was available.

I believe to not violate any patents or laws with the creation and use of
this work. However, your local laws might tell you something different. Be
careful!

HOW TO START:

Please take a look at xexample.c. It's commented, and displays a simple
spinning cube with texture.

Also, you might want to look at xextex.c, which contains a 16bit texture,
stencil buffer operations and makes optionally use of alphablending. 

MICROCODES:

Due to legal reasons, two required microcode files (ucode0.bin, ucode1.bin)
could not be included in this package.

However, a seperately released tool called "romextract" extracts them from
the kernel stored in the flashrom. You need those two files to actually run
the demonstration program.

LICENSE:

This code is, for now, licensed for non-commercial use only. If you prefer
another license, contact me. Whatever. Take care about the gl/glu ripped
parts. In doubt, remove them first.

This code currently runs on linux. It requires access to /dev/mem, so
running it as root is usually required. It communicates directly with the
graphic hardware, so no userspace library is required.

PERFORMANCE NOTE:

However, mmap'ing /dev/mem maps the memory as uncached. This is not
changeable - O_SYNC only forces uncached mapping when mapping memory below
the memory limit (which is 0x1e000000). So we are out of luck.

Anyway, you can patch the kernel to remap memory cached, or even better,
write combined. This library will use proper cache instructions, so modify
the kernel in any way you like, all three kinds of memory should work
(uncached, cached, write combined). Write-combined memory is definitely
preferred, cached memory is also ok, uncached memory is SLOW (by a factor of
about 100x. yes.)

The only critical data are command buffer data, and while using uncached
memory is damn slow, the command buffers are not *that* big. But depending
on the amount of data, you can get a nice performance gain my modifying the
memory mapping to a proper type. For a start, the default mapping is fine.
Just don't expect a good performance.


Do you need more space for textures etc.? Unless you want to write a
physical memory allocator, just boot linux with less ram (for example,
128M). Then, adjust the "RINGBUFFER_START" in xe.c.

So, how does it work?

We have a standard 3D acceleration device. Nothing is really too special. If
you know how GPUs work, then you know how the 360 GPU works.

Fortunately, we have a GPU which doesn't provide much fixed-functions. Most
stuff is done in vertex and pixel shaders. Some stuff, however, is still
hardcoded, like blending, z/stencil-compare. 

The typical sequence to render a frame is:

for each model:
  calculate and load shader constants ("matrices")
  for each renderstate:
    setup renderstate
    for each mesh using this renderstate:
      draw mesh
resolve to framebuffer while clearing EDRAM

The xenos GPU can only render to the embedded framebuffer (EDRAM). The EDRAM
contains both the framebuffer as well as the z-buffer. After a frame has been
rendered, it must be "resolved" into the main RAM to be displayed. While
resolving the EDRAM is also cleared with a constant color (and constant Z
value), so it's ready for the next frame.

All data accessed by the GPU must lie in physical memory. Currently, we
don't have a defined kernel interface, so we just use the memory at
0x1e000000 upwards, which is currently reserved (it contains the
framebuffer, but nothing more).

The xenos works with a continous stream of commands ("command buffer").
Commands are usually just register writes, but are processed automatically.
A command buffer can call an "indirect buffer" (or sub command-buffer),
which is used to automatically enqueue commands, even though the main
command buffer ringsize is limited to 32kb.

Then you need shaders. As I don't dare to write a shader assembler or even
compiler (beware!), we just use Microsoft's XNA to do the job. Tser wrote a
wrapper for the provided native functionality, so you can compile your HLSL
shaders using his tool (you need XNA installed - but you don't need XNA on
the xbox, don't worry).

MSAA doesn't work yet. So does Alphatest.

The API was made to resemble the hardware as close as possible, while still
maintaining a useful abstraction level. The hardware resembles Direct3D - so
my API does as well. No, this was not done because the original Xbox
software uses D3D.

API DOC: (haha)

struct XenosShader

resembles a shader. A vertexshader is bound to the vertex buffer format,
that's why you might need more than one "instance" for a vertex shader (if
you want to use different VBFs with the same shader). That's where the
instances are for - you load the shader once, then instantiate it for every
VBF, then you apply the vfetch patches. Look into the example.

Pixel shader don't need that. Just always use instance 0 here.

struct XenosVBFElement and struct XenosVBFFormat

describe a vertex buffer format, to be used when applying vfetch patches.

struct XenosSurface

describes a surface, be it a texture or a render target. pitch is the line
distance in pixel. format is one of the XE_FMT_* stuff, possibly with an
endianess modifier. Use "Xe_Create_Surface" to allocate a surface, or
"Xe_GetFramebufferSurface" to get the framebuffer surface.

A surface can be locked, if you want access to the data in it. See the
"xextex.c" example. You can also lock a surface for reading after resolving
into it. Make sure that you Xe_Sync before, otherwise not all content might
be rendered/written to memory.

struct XenosVertexBuffer

describes a vertexbuffer, to be passed to DrawPrimitive /
DrawIndexedPrimitive. Create with Xe_CreateVertexBuffer.

struct XenosIndexBuffer

desribes an index buffer. Can be either 16 or 32bit indices.

struct XenosDevice

describes an opaque structure you should never need to mess around with.
Unless you have a very good reason, use the functions.

void Xe_Init(struct XenosDevice *xe);

Initializes the hardware.

void Xe_SetRenderTarget(struct XenosDevice *xe, struct XenosSurface *rt);

Set render target for the next resolve. Also sets viewport size.

void Xe_Resolve(struct XenosDevice *xe);

Resolve the edram color buffer into memory, and clear the framebuffer. 

void Xe_ResolveInto(struct XenosDevice *xe, struct XenosSurface *surface, int source, int clear);

Resolve into a surface, with some more control.

Source can be XE_SOURCE_COLOR (for copying the color buffer), or XE_SOURCE_DS (for copying
the depth/stencil buffer). You can selectively clear the color- or
depth/stencil-buffer with XE_CLEAR_COLOR and XE_CLEAR_DS.

void Xe_Clear(struct XenosDevice *xe, int flags);

Clears the edram, either color, depth/stencil or both.

struct XenosSurface *Xe_GetFramebufferSurface(struct XenosDevice *xe);

Gets a pointer to the surface describing the framebuffer. You should set
this as a rendertarget, unless of course you don't want.

void Xe_Execute(struct XenosDevice *xe);

Kicks the GPU.

void Xe_Sync(struct XenosDevice *xe);

Kicks the GPU, and waits until it executed all stuff queued up before.

void Xe_SetClearColor(struct XenosDevice *xe, u32 clearcolor);

Set the color to clear on resolve / clear.

void Xe_DirtyAluConstant(struct XenosDevice *xe, int base, int len);

In case you directly overwrite a shader constant in the XenosDevice-struct,
you need to flag them dirty. Or just use "SetShaderConstant" to make things
simpler.

void Xe_DirtyFetch(struct XenosDevice *xe, int base, int len);

Same for fetch constants. You shouldn't mess around with them, though.

struct XenosShader *Xe_LoadShader(struct XenosDevice *xe, const char *filename);

Loads a shader from disk.

struct XenosShader *Xe_LoadShaderFromMemory(struct XenosDevice *xe, void *shader);

Loads a shader from memory. Please not that the memory belongs to the device
afterwards - don't clear/free it!

void Xe_InstantiateShader(struct XenosDevice *xe, struct XenosShader *sh, unsigned int index);

"Instantiate" a shader, i.e. copy it to physical memory.

int Xe_GetShaderLength(struct XenosDevice *xe, void *sh);

If you need, you can calculate a shader length with this call.

void Xe_ShaderApplyVFetchPatches(struct XenosDevice *xe, struct XenosShader *sh, unsigned int index, const struct XenosVBFFormat *fmt);

Important. You need to fixup vertex shader to your used vertexbuffer format.
See the examples.

int Xe_VBFCalcStride(struct XenosDevice *xe, const struct XenosVBFFormat *fmt);
int Xe_VBFCalcSize(struct XenosDevice *xe, const struct XenosVBFElement *fmt);

Some helper functions if you mess around with VBFs.

The following renderstate functions will try to resemble d3d. Please see
there for details.

void Xe_SetZFunc(struct XenosDevice *xe, int z_func);

Set the Z compare function.

void Xe_SetZWrite(struct XenosDevice *xe, int zw);

Enable or disable Z write.

void Xe_SetZEnable(struct XenosDevice *xe, int zw);

Enable or disable Z compare.

void Xe_SetFillMode(struct XenosDevice *xe, int front, int back);

Set the fill mode for front- and backfacing geometry (point, line, solid,
...).

void Xe_SetBlendControl(struct XenosDevice *xe, int col_src, int col_op, int col_dst, int alpha_src, int alpha_op, int alpha_dst);

Set all blend controls at once.

void Xe_SetSrcBlend(struct XenosDevice *xe, unsigned int blend);

Set the source blend factor.

void Xe_SetDestBlend(struct XenosDevice *xe, unsigned int blend);

Set the dest blend factor.

void Xe_SetBlendOp(struct XenosDevice *xe, unsigned int blendop);

Set the blend operation.

void Xe_SetSrcBlendAlpha(struct XenosDevice *xe, unsigned int blend);
void Xe_SetDestBlendAlpha(struct XenosDevice *xe, unsigned int blend);
void Xe_SetBlendOpAlpha(struct XenosDevice *xe, unsigned int blendop);

same, just for alpha.

void Xe_SetCullMode(struct XenosDevice *xe, unsigned int cullmode);

Set the cullmode. If you select CW, the definition of front and backfaces
will be inverted.

void Xe_SetAlphaTestEnable(struct XenosDevice *xe, int enable);
void Xe_SetAlphaFunc(struct XenosDevice *xe, unsigned int func);
void Xe_SetAlphaRef(struct XenosDevice *xe, float alpharef);

This doesn't work yet.

	/* bfff is a bitfield {backface,frontface} */
void Xe_SetStencilEnable(struct XenosDevice *xe, unsigned int enable);

Enable or disable stencil operation. you need this for both stencil compare
and stencil modification.

void Xe_SetStencilFunc(struct XenosDevice *xe, int bfff, unsigned int func);

Set the stencil compare function. bfff is a bitfield: 1 for front facing
polys, 2 for back facing polys, so you can set that individually. 


	/* -1 to leave old value */
void Xe_SetStencilOp(struct XenosDevice *xe, int bfff, int fail, int zfail, int pass);

Set the stencil operation, for stencil-failed polys, z-failed polys, and
passed polys.

void Xe_SetStencilRef(struct XenosDevice *xe, int bfff, int ref);

Set the stencil compare reference value.

void Xe_SetStencilMask(struct XenosDevice *xe, int bfff, int mask);

Set the stencil compare mask.

void Xe_SetStencilWriteMask(struct XenosDevice *xe, int bfff, int writemask);

Set the stencil write/modify mask.

void Xe_InvalidateState(struct XenosDevice *xe);

Should be called at the beginning of each frame - sets all renderstates to 
reasonable defaults.

void Xe_SetShader(struct XenosDevice *xe, int type, struct XenosShader *sh, int instance);

Set a shader to use, either the pixel or the vertex shader, with the given
instance (for vs).

void Xe_SetTexture(struct XenosDevice *xe, int index, struct XenosSurface *tex);

Set a texture to be used in a sampler.

struct XenosVertexBuffer *Xe_VBPoolAlloc(struct XenosDevice *xe, int size);
void Xe_VBPoolAdd(struct XenosDevice *xe, struct XenosVertexBuffer *vb);
void Xe_VBReclaim(struct XenosDevice *xe);
void Xe_VBBegin(struct XenosDevice *xe, int pitch); /* pitch, len is nr of vertices */
void Xe_VBPut(struct XenosDevice *xe, void *data, int len);
struct XenosVertexBuffer *Xe_VBEnd(struct XenosDevice *xe);
void Xe_Draw(struct XenosDevice *xe, struct XenosVertexBuffer *vb, struct XenosIndexBuffer *ib);

You should not use these functions, unless you want dynamic vertexbuffer the
ugly way.

void Xe_SetIndices(struct XenosDevice *de, struct XenosIndexBuffer *ib);

Set the indexbuffer to use for Xe_DrawIndexedPrimitive.

void Xe_SetStreamSource(struct XenosDevice *xe, int index, struct XenosVertexBuffer *vb, int offset, int stride);

Set the vertexbuffer to use for Xe_Draw[Indexed]Primitive. If you want, you
can specify an offset. Stride is ignored so far.

void Xe_DrawIndexedPrimitive(struct XenosDevice *xe, int type, int base_index, int min_index, int num_vertices, int start_index, int primitive_count);
void Xe_DrawPrimitive(struct XenosDevice *xe, int type, int start, int primitive_count);

Draw primitives, either with indexbuffer or without.

struct XenosIndexBuffer *Xe_CreateIndexBuffer(struct XenosDevice *xe, int length, int format);

Create an indexbuffer with the given length and format.

struct XenosVertexBuffer *Xe_CreateVertexBuffer(struct XenosDevice *xe, int length);

Create a vertexbuffer with the given length.

void *Xe_VB_Lock(struct XenosDevice *xe, struct XenosVertexBuffer *vb, int offset, int size, int flags);
void *Xe_IB_Lock(struct XenosDevice *xe, struct XenosIndexBuffer *ib, int offset, int size, int flags);
void *Xe_Surface_LockRect(struct XenosDevice *xe, struct XenosSurface *surface, int x, int y, int w, int h, int flags);

Lock a vertex/indexbuffer/surface. Use flags=XE_LOCK_WRITE if you want to
write, or XE_LOCK_READ for reading.

void Xe_VB_Unlock(struct XenosDevice *xe, struct XenosVertexBuffer *vb);
void Xe_IB_Unlock(struct XenosDevice *xe, struct XenosIndexBuffer *ib);
void Xe_Surface_Unlock(struct XenosDevice *xe, struct XenosSurface *surface);

After locking, you need to unlock. This will automatically flush the CPU
cache.

void Xe_SetVertexShaderConstantF(struct XenosDevice *xe, int start, const float *data, int count); /* count = number of 4 floats */
void Xe_SetPixelShaderConstantF(struct XenosDevice *xe, int start, const float *data, int count); /* count = number of 4 floats */

Set a pixel/vertex shader constant.

struct XenosSurface *Xe_CreateTexture(struct XenosDevice *xe, unsigned int width, unsigned int height, unsigned int levels, int format, int tiled);

Create a surface in physical memory.


CHANGELOG:

0.0.1: 
	* initial pre-release
0.0.2: 
	* new example, much cleanup
0.0.3: 
	* load ucodes from disk
0.0.5:
	* major API overhaul
	* some additional features
	* many, many bugfixes

All the stuff (except when noted otherwise) was written by me, 
Felix Domke <tmbinc@elitedvb.net>
