#!/bin/bash

mkdir -p build
pushd build
cmake ..
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
mv compile_commands.json ..
popd
