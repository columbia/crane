GCC := gcc
CLAGS := -std=gnu11 -Wall -g -I"../../.local/include"
LDFLAGS := -L"../../.local/lib" -ldb



a.out:main.c
	$(GCC) $(CLAGS) -o $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	@rm -rf a.out
