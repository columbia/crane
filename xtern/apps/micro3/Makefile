CFLAGS = -funroll-loops -fprefetch-loop-arrays -fpermissive -fno-exceptions -DENABLE_THREADS -I$(XTERN_ROOT)/include
LDFLAGS = -L$(XTERN_ROOT)/dync_hook -Wl,--rpath,$(XTERN_ROOT)/dync_hook
LIBS = -lstdc++ -lpthread -lxtern-annot
all: micro3

micro3: micro3.cpp
	g++ micro3.cpp -g -O3 -o micro3 $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -rf micro3
