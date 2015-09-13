CC=gcc-4.8
LIB_NAME="./libclilib.so"
CFLAGS := -Wall -O2 -g -std=gnu11 -I../.local/include
LDFLAGS= -L"$(MSMR_ROOT)/libevent_paxos/.local/lib" -lpthread -lrt -ldl -lconfig

target = libclilib.so.1.0 \

all:$(target)

libclilib.so.1.0:pctern.o enter_sys.o config-client.o
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
