#!/bin/bash
echo "Building trader"

BUILD_DIR="../build/"
SRC_DIR="../src"

BIN_NAME="trader"

INCLUDES="-I ../src/submodules/stb/ -I ../src/submodules/openssl/include"
SOURCES="$SRC_DIR/platform_linux/linux_trader.cpp"

DEBUG_FLAGS="-Wall -Werror -Wextra -Wdouble-promotion -Wformat -Wformat-overflow -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers -Wunused -Wuninitialized"

if [ ! -d "$BUILD_DIR" ]; then
  mkdir $BUILD_DIR
fi

pushd $BUILD_DIR
  g++ $SOURCES -time -o $BIN_NAME $DEBUG_FLAGS -fdiagnostics-show-caret -g -ggdb -gdwarf -g3 -Og -p -fno-exceptions -fno-stack-protector -fno-rtti -save-temps $INCLUDES -nostdinc++

  chmod +x $BIN_NAME
popd

