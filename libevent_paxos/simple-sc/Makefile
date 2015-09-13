GCC := gcc-4.8
CFLAGS := -Wall -O2 -g -std=gnu11 -I../.local/include
LDFLAGS := 

SOURCES := $(shell find . -name "*.c")
OBJS    := $(SOURCES:.c=.o)
DEPS    := $(SOURCES:.c=.d)

DIR := $(shell pwd)

.PHONY: clean list all run

PROGRAM := simple-client.out \
		   simple-server.out

all:$(PROGRAM)

-include ${DEPS}

%.out:%.o
	$(GCC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
	
.c.o:
	$(GCC) $(CFLAGS) -MM -MF $(patsubst %.o,%.d,$@) $^
	$(GCC) $(CFLAGS) -c -o $@ $^

clean:
	@rm -rf $(OBJS)
	@rm -rf $(PROGRAM)
	@rm -rf $(DEPS)

list:
	@echo $(SOURCES)
	@echo $(OBJS)
	@echo $(DEPS)
run:
	@$(DIR)/client.out -n 0 -s 127.0.0.1 -p 9000 -r 1000 &
	@$(DIR)/client.out -n 1 -s 127.0.0.1 -p 9000 -r 1000 &
	@$(DIR)/client.out -n 2 -s 127.0.0.1 -p 9000 -r 1000 &

