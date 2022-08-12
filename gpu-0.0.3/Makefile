all: gpu

GPU_OBJS = ioremap.o gpu.o engine.o xenos_init.o xenos_global_init.o \
	xenos_func.o gpu_matrix.o gpu_resolve.o gpu_state.o \
	gpu_shader.o gpu_vb.o gpu_rb.o gpu_mem.o

LDFLAGS = -lm
CFLAGS = -Wall -g  -O2

gpu: $(GPU_OBJS)
	gcc $(GPU_OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f *.o
