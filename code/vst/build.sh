#!/bin/bash

###########
INIT_DIR=$( pwd )
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

cmake -S . -B build
cmake --build build -j8

ambilink_dir=build/Ambilink.vst3/Contents/x86_64-linux/
mkdir -p $ambilink_dir
cp build/lib/Ambilink.so $ambilink_dir/Ambilink.so

cd $INIT_DIR

