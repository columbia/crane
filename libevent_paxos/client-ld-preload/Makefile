CC=gcc-4.8
LIB_NAME="./libclilib.so"
LDFLAGS=-lpthread -lrt -ldl

target = libclilib.so.1.0 \

all:$(target)

libclilib.so.1.0:pctern.o enter_sys.o
	$(CC) $^ -Wl,-soname,libclilib.so.1 -rdynamic -shared -fpic -o $@ $(LDFLAGS) 

.c.o:
	$(CC) $(CFLAGS) -fpic -g -c -o $@ $^


.PHONY: clean link run

link:
	ln -sf libclilib.so.1.0 libclilib.so.1
	ln -sf libclilib.so.1.0 libclilib.so

clean:
	rm -rf *.o
	rm -rf *.out
	rm -rf libclilib*
	rm -rf $(target)
	rm -rf *.log
