all::
	g++ ex2.c -o ex2 -lpthread -I$(XTERN_ROOT)/include \
	-L$(XTERN_ROOT)/dync_hook -Wl,--rpath,$(XTERN_ROOT)/dync_hook -lxtern-annot \
	-lpthread
	g++ ex3.c -o ex3 -lpthread -I$(XTERN_ROOT)/include \
        -L$(XTERN_ROOT)/dync_hook -Wl,--rpath,$(XTERN_ROOT)/dync_hook -lxtern-annot \
        -lpthread
