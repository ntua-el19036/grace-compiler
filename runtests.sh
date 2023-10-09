#!/bin/bash

for file in ./test/ntua_compilers/grace/programs/*.grc
do
    echo "Checking $file : "
    ./compiler < $file
done
