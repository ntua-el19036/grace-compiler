# grace-compiler

LLVM version 11.1.0
flex 2.6.4
bison (GNU Bison) 3.8.2
clang version 11.1.0-6

#Building the Compiler
Build the compiler with:
```
make
```

## Running the Compiler
Run the compiler with:
```
./gracec [-f | -i] <source_file>
```

Use the `-f` flag to get the assembly code in stdout.
Use the `-i` flag to get the llvm code in stdout.
Do not use any flags to get a `<source_file>.asm` and `<source_file>.imm` file (in the same
folder as the source code) containing the assebly and llvm code respectively.

To run a program, you can generate an executable using the `./do.sh` script. It creates a `a.out` executable in the
current working directory.
```
./do.sh <source_file>
./a.out
```
