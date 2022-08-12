# GPU demos for Linux on the Xbox 360, by [tmbinc](https://github.com/tmbinc)

## 2007-09-28 - [Fear, triangles!](https://debugmo.de/2007/09/fear-triangles/)
I’m proud to present:

![3daccel.jpg](./img/3daccel.jpg?raw=true)

Triangles on the 360, MANY OF THEM - about 40 million per second, or even more if you write clever code. (But this is not a depth of field, just a blurry screenshot ;)

I finally polished my GPU stuff far enough so I can risk a release. You need to compile your shaders, so you need Tser’s shader compiler (which uses part of the windows XNA libraries).

So, without further words: [`gpu-0.0.3/`](./gpu-0.0.3) and [`romextract-0.0.1/`](./romextract-0.0.1) (to extract the required ucodes from flash). Have fun!

Ok, some further words: I didn’t implemented any standard API yet, right now, the functions are on a very low level, directly above the hardware. Just take a look at [`gpu.c`](./gpu-0.0.3/gpu.c), and try to understand the example code. Then you should be able to do something more.

You might also be interested in xenkit, which is based on gpu, and adds X11 support and some demos.

## 2008-03-27 - [Xbox 360 GPU update](https://debugmo.de/2008/03/xbox-360-gpu-update/)

First, here are the promised slides for my Breakpoint 2008 presentation about “Gaming Consoles for demosceners”: [breakpoint-2008-slides.pdf](./img/breakpoint-2008-slides.pdf)

Then, I’ve updated my GPU library a bit. The biggest thing was a rewrite of the interface, so now it’s all encapsulated into a nice API. I’ve also added some features (stencil buffer ops, drawing with index buffers), and fixed a LOT of bugs (for example vfetch patches on more complex shaders). The updated GPU library, included the mentioned “spinning cube” example, is available here: [`gpu-0.0.5/`](./gpu-0.0.5).

Also, I’ve ported some existing code to my library. A simple test scene looks like this:

![xenos_sample.jpg](./img/xenos_sample.jpg?raw=true)

This is a per-pixel lighting shader with stencil-based shadow volumes - and obviously 3 cubes. I thought it looked nice enough to put it here.

