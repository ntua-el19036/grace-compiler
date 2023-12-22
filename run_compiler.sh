#!/bin/bash

# Step 1: Clean the project
make distclean

# Step 2: Build the compiler
make compiler

# Step 3: Run the compiler with input from test.grc
./compiler < test.grc
