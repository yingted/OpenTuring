#!/bin/sh
#craps on the code
find vc\ projects/ -name CMakeLists.txt -execdir cmake -DCMAKE_MODULE_PATH=/home/ted/box/OpenTuring/turing/cmake/Modules . \;
