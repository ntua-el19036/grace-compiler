CC=gcc
CFLAGS=-Wall

default: compiler

lexer: lexer.l
	flex -o lexer.c lexer.l
	$(CC) $(CFLAGS) -o lexer lexer.c

compiler: lexer.o parser.o
	$(CC) $(CFLAGS) -o $@ $^ -lfl

lexer.c: lexer.l
	flex -s -o $@ $<

lexer.o: lexer.c lexer.h parser.h

parser.c parser.h: parser.y
	bison -dv -o $@ $<

parser.o: parser.c parser.h

clean:
	$(RM) *.o parser.c parser.h lexer lexer.c core *~

distclean: clean
	$(RM) compiler
