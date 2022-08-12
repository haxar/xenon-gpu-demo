all: xexample screenshot

XEXAMPLE_OBJS = ioremap.o xe.o xee.o engine.o xexample.o
SCREENSHOT_OBJS = ioremap.o screenshot.o
# xextex.o

LDFLAGS = -lm
CFLAGS = -Wall -g  -O2

xexample: $(XEXAMPLE_OBJS)
	gcc $(XEXAMPLE_OBJS) -o $@ $(LDFLAGS)

screenshot: $(SCREENSHOT_OBJS)
	gcc $(SCREENSHOT_OBJS) -o $@ $(LDFLAGS) -lpng

clean:
	rm -f $(XEXAMPLE_OBJS) $(SCREENSHOT_OBJS) screenshot xexample
