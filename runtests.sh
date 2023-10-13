#!/bin/bash

for file in ./test/ntua_compilers/grace/programs/*.grc
do
    echo "Checking $file : "
    ./compiler < $file
done

for file in ./test/ntua_compilers/grace/programs-erroneous/*.grc
do
    echo "Checking erroneous $file : "
    ./compiler < $file
done
