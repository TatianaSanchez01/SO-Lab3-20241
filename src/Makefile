SRCS = saxpy.c
TARG = saxpy
OBJS = $(SRCS:.c=.o)

CC = gcc
OPTS = -Wall -O3
LIBS = -lpthread

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    OPTS = -g -DDEBUG
endif

$(TARG): $(OBJS)
	$(CC) -o $(TARG) $(OBJS) $(LIBS) 

#-fopenmp

%.o: %.c
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARG) *~