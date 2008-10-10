BUILD=/home/james/Code/C/Linux/jimix/jimix/build
CXX=$(BUILD)/g++
CC=$(BUILD)/gcc
CFLAGS=-fno-builtin -m32 -I../../src/lib/include
SOURCES=lex.yy.c parser.tab.c shell.o lex.yy.o parser.tab.o

all: $(SOURCES) link
clean:
	-rm *.o parser.tab.c parser.tab.h lex.yy.c shell

link:
	$(CC) -o shell shell.o lex.yy.o parser.tab.o /usr/lib/libfl.a $(CFLAGS)

lex.yy.c:
	flex lexer.l

parser.tab.c:
	bison -d parser.y

lex.yy.o: lex.yy.c parser.tab.o
	$(CC) -o lex.yy.o -c lex.yy.c $(CFLAGS)

parser.tab.o: parser.tab.c
	$(CC) -o parser.tab.o -c parser.tab.c $(CFLAGS)

.c.o:
ifndef LOUD
	@echo "[$(@F)]"
else
	@echo "$(CC) $(CFLAGS) -o $@ -c $<"
endif
	@$(CC) $(CFLAGS) -o $@ -c $<