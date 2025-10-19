#!/bin/bash

cmake --build build

mv ./build/compile_commands.json .

./build/hello

