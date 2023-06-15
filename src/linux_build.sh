#!/bin/bash
echo "Building trader"

BUILD_DIR="../build/"

echo $BUILD_DIR
if [ ! -d "$BUILD_DIR" ]; then
  mkdir $BUILD_DIR
fi

# -fstrict-aliasing -Wstrict-aliasing 

pushd $BUILD_DIR
  g++ ../src/platform_linux/linux_trader.cpp -time -o trader.bin -Wall -Werror -Wextra -Wdouble-promotion -Wformat -Wformat-overflow -Wunused -Wuninitialized -fdiagnostics-show-caret -g -ggdb -gdwarf -g3 -Og -fwhole-program -p -fno-exceptions -fno-stack-protector -fno-rtti -S -Wno-unused-function
popd
