CFLAGS=-O3
LDFLAGS=-lc -lm

CC=gcc
LD=ld

SRCS:=test.c
OBJS:=test.o

OUTPUT:=test

all: $(OUTPUT)

$(OUTPUT) : $(OBJS)
	$(LD) -o $(OUTPUT) $(OBJS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
