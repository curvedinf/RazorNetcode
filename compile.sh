#!/bin/sh

echo "compiling..."
g++ -std=c++20 -fmodules-ts -c src/includes.cc
g++ -std=c++20 -fmodules-ts -c src/math.cc
#g++ -std=c++20 -fmodules-ts -c src/misc.cc
#g++ -std=c++20 -fmodules-ts -c src/serialization.cc
#g++ -std=c++20 -fmodules-ts -c src/networking.cc
echo "linking..."
#g++ -std=c++20 -fmodules-ts -shared includes.o math.o serialization.o misc.o \
#-o librazor.so -lpthread -lSDL2 -lSDL2_net -lcurl
echo "cleaning up..."
rm *.o
rm -r gcm.cache
#g++-11 -std=c++20 -fmodules-ts main.cpp -o app -Xlinker ./libmathlib.so