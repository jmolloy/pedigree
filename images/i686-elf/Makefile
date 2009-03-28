CFLAGS=-O3
LDFLAGS=

CC=gcc
LD=gcc

SRCS:=test.c
OBJS:=test.o

OUTPUT:=test

all: $(OUTPUT)

$(OUTPUT) : $(OBJS)
	$(LD) $(OBJS) -o $(OUTPUT)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
