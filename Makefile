CC=gcc
CFLAGS=-Wall

lexer: lexer.l
	flex -o lexer.c lexer.l
	$(CC) $(CFLAGS) -o lexer lexer.c

compiler: lexer.o parser.o symbol.o general.o error.o symbol.o
	$(CC) $(CFLAGS) -o $@ $^ -lfl
	
lexer.c: lexer.l parser.h
	flex -s -o $@ $<
	
parser.c parser.h: parser.y
	bison -dv -o $@ $<
clean:
	$(RM) *.o parser.c parser.h lexer lexer.c core *~
distclean: clean
	$(RM) compiler
