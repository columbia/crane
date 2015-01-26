GCC := gcc
CLAGS := -std=gnu11 -Wall -g
LDFLAGS := 



a.out:main.c
	$(GCC) $(CLAGS) $(LDFLAGS) -o $@ $^ 

.PHONY:clean
clean:
	@rm -rf a.out
