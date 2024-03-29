CXX=c++
CXXFLAGS=-Wall -std=c++14 `llvm-config-11 --cxxflags`
LDFLAGS=`llvm-config-11 --ldflags --system-libs --libs all`

default: gracec

lexer.cpp: lexer.l
	flex -s -o lexer.cpp lexer.l

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

lexer.o: lexer.cpp lexer.hpp parser.hpp ast.hpp symbol.hpp

parser.cpp parser.hpp: parser.y
	bison -dv -t -o parser.cpp parser.y

parser.o: parser.cpp lexer.hpp ast.hpp symbol.hpp

gracec: lexer.o parser.o ast.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.output *.s *.out *.ll *.asm *.imm *.o parser.cpp parser.hpp lexer lexer.cpp core *~

distclean: clean
	$(RM) gracec
