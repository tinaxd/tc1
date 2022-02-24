CFLAGS=-std=c11 -g -static

tc1: tc1.c

test: tc1
	./test.sh

clean:
	rm -f tc1 *.o *~ tmp*

.PHONY: test clean
