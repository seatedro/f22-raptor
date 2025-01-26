#!/bin/bash
curl -L https://github.com/emscripten-core/emsdk/archive/main.tar.gz -o emsdk.tar.gz
tar -xf emsdk.tar.gz
cd emsdk-main
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..
mkdir build
cd build
emcmake cmake ..
emmake make