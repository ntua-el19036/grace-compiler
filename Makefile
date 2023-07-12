CXX=c++
CXXFLAGS=-Wall

default: compiler

lexer: lexer.l
	flex -o lexer.cpp lexer.l
	$(CXX) -o lexer lexer.cpp

compiler: lexer.o parser.o
	$(CXX) $(CXXFLAGS) -o $@ $^

lexer.cpp: lexer.l
	flex -s -o $@ $<

lexer.o: lexer.cpp lexer.hpp parser.hpp ast.hpp symbol.hpp

parser.cpp parser.hpp: parser.y
	bison -dv -t -o $@ $<

parser.o: parser.cpp parser.hpp ast.hpp symbol.hpp

clean:
	$(RM) *.o parser.cpp parser.hpp lexer lexer.cpp core *~

distclean: clean
	$(RM) compiler
