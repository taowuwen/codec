
CFLAGS = -Wall
LDFLAGS=-pthread

SRCS=mempool.c mem.c test.c
OBJS=$(patsubst  %.c,%.o, $(SRCS))

all: test
	@echo srcs: $(SRCS)
	@echo objs: $(OBJS)

test: $(OBJS)
	gcc -o $@ $^ $(LDFLAGS)

.c.o:
	gcc -c $(CFLAGS) $<

clean:
	@rm -f *.o
	@rm test
