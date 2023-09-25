CXX=c++
CXXFLAGS=-Wall -std=c++14 `llvm-config --cxxflags`
LDFLAGS=`llvm-config --ldflags --system-libs --libs all`

default: compiler

lexer.cpp: lexer.l
	flex -s -o lexer.cpp lexer.l

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

lexer.o: lexer.cpp lexer.hpp parser.hpp ast.hpp symbol.hpp

parser.cpp parser.hpp: parser.y
	bison -dv -t -o parser.cpp parser.y

parser.o: parser.cpp lexer.hpp ast.hpp symbol.hpp 

compiler: lexer.o parser.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o parser.cpp parser.hpp lexer lexer.cpp core *~

distclean: clean
	$(RM) compiler
