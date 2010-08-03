all: lexer.c parser.c
	gcc -DNOT_PEDIGREE -o keymap parser.c lexer.c main.c

lexer.c: lexer.l
	lex -o lexer.c lexer.l

parser.c: parser.y
	yacc -d -o parser.c parser.y
