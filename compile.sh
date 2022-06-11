#!/bin/sh

g++ -c -std=c++20 -fmodules-ts src/math.cc src/serialization.cc \
src/misc.cc src/networking.cc