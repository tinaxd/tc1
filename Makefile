CFLAGS=-std=c11 -g -static -Wall -Wextra
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

tc1: $(OBJS)
	$(CC) -o tc1 $(OBJS) $(LDFLAGS)

$(OBJS): tc1.h

test: tc1
	./test.sh

clean:
	rm -f tc1 *.o *~ tmp*

.PHONY: test clean
