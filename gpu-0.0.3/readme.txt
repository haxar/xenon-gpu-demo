Xenos 3D Library

This code resembles a library to use the 3d functionality of the 'xenos'
chip in the Xbox 360.

It was written from scratch, and contains no microsoft code. All information
were reverse engineered from either code or ringbuffer dumps. 
No chip documentation was available.

I believe to not violate any patents or laws with the creation and use of
this work. However, your local laws might tell you something different. Be
careful!

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

The library consists of different parts, some of them are hardware related,
some of them are not.

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



Some sourcecode notes:

engine.c: 
  some high-level matrix helper functions, nothing spectacular. 
  some things were ripped from mesa/glu (gluLookAt, glRotate), some other
  things are just suboptimal or broken. I'm really not proud of this.

gpu.c:
  The main function. still contains much too much stuff. we have a hardcoded
  framebuffer base here, which isn't even considered in memory allocation.

  we have a "draw" function here, which takes a (possibly linked) vertex
  buffer, and emits "DrawNonIndexed" draw commands.

  Then we have our test demo, a torus. Isn't it great, this torus? You can
  adjust it STEP_U, STEP_V, but it's all not that cute.

  We have our main function, which

    - ioremaps the hardware registers,
    - ioremaps the "ringbuffer" (which also contains all other
      physical-memory objects, like shaders, textures etc.)
    - 'resolve' will be initialized (RS will allocate vertex buffers)
    - c01 is a hardcoded cube, my first-ever test object.
    - 'tri' (or vb_tri) is a triangle,
    - it will also initialize the "vertex buffer format", and apply it to
      the triangle shader. same for the textured triangle shader,
    - will read the test texture ("texture.raw") into tex_test,
    - initializes the ringbuffer hardware,
    - sets up the ringbuffer writeback as well as the "scratch registers",
    - sets up a simple perspective matrix,
    - and starts to draw the torus in the while-loop.

gpu_matrix.c:
  Handles matrix upload. I like RH systems, but the hardware wants LH, so i
  convert this here.

gpu_mem.c:
  Physical memory handling. The cache flush operations are only important if
  you reconfigure ioremap-memory to be cachable. Note that there is no
  "MEM_Free", because I was too lazy for that.

gpu_rb.c:
  The ringbuffer handling functions. There general approach is to have a
  real primary ringbuffer, and a secondary buffer. Commands push into the
  secondary buffer, and at certain amounts (or when forced), these segments
  will be called with a "call" command written into the primary
  (size-limited) buffer.

gpu_resolve.c:
  Functions to resolve the EDRAM into memory. Mostly ripped 1:1 from an
  existing command buffer.

gpu_shader.c:
  Parses XNA-generated shader files, and patches vfetch-instructions to
  match the specified vertex buffer.

gpu_state.c:
  GPU state manipulation. Handles stuff like different z-write modes etc.

gpu_vb.c:
  Dynamic vertex buffer support. In general, you should avoid having dynamic
  vertex buffers (buffers which are modified from frame to frame), but with
  these functions, you can easily alloc it, push vertices into it, and then
  get a linked list of the vertices, so you can draw them. VBs which are
  used should be added to the pool, so they can be reclaimed after the
  current frame.

xenos_func.c:
  Some required functions which need some cleanup.

xenos_global_init.c, xenos_init.c:
  Ugly. One-time init.

*.hlsl:
  The example shaders. You need to compile them using tser' compiler.

PERFORMANCE WARNING:
Yes, the GPU is quoted at approx. 10x the speed which this little test
program can archive. Note that there are a dozen things to optimize, and
this is a first version. So don't go and tell the world: "it's SLOW!" -
instead, find the performance bottlenecks and improve them. There are a LOT
of things to improve.

TODO:
Reverse Engineering Tasks:
 - draw with index buffer
 - draw non-triangles (trilists, stripes, quads, ...)
 - find out more texture formats, compressed textures, ...
 - exploit all the cool fixed-function GPU features

Software Design Tasks:
 - improve/rewrite the API
 - gl implementation, someone?
 - remove the linux dependency

Boring Tasks:
 - sound support

CHANGELOG:

0.0.1: 
	* initial pre-release
0.0.2: 
	* new example, much cleanup
0.0.3: 
	* load ucodes from disk

Now, who is the first one who writes a wiimote-driven 3d game for the 360?

All the stuff (except when noted otherwise) was written by me, 
Felix Domke <tmbinc@elitedvb.net>
