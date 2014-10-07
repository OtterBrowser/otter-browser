#! /bin/sh

cd ../
mkdir build
cd build
cmake ../
make
cpack ./
