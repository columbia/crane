CFLAGS = -funroll-loops -fprefetch-loop-arrays -fpermissive -fno-exceptions -DENABLE_THREADS -I$(XTERN_ROOT)/include
LDFLAGS = -L$(XTERN_ROOT)/dync_hook -Wl,--rpath,$(XTERN_ROOT)/dync_hook
LIBS = -lstdc++ -lpthread -lxtern-annot
all: micro

micro: micro.cpp
	g++ micro.cpp -o micro $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -rf micro
