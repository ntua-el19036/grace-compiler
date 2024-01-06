#!/bin/bash

./gracec -i $1 > a.ll
llc-11 -o a.s a.ll
clang-11 -o a.out a.s lib.a
