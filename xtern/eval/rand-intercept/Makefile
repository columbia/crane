all:: install

install::
	gcc -rdynamic -Wall -shared -fpic -o rand-intercept.so rand-intercept.c -lpthread -lrt -ldl
