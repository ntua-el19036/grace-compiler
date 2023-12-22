#!/bin/bash

llc-11 -o a.s $1
clang-11 -o a.out a.s lib.a