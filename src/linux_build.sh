#!/bin/bash
echo "Building trader"

BUILD_DIR="../build/"
SRC_DIR="../src"

BIN_NAME="trader"

INCLUDES="-I../src/submodules/stb/ -I../src/submodules/openssl/include"

LIBRARY_PATHS="-L/usr/local/ssl/lib"
LIBRARIES="-lX11 -lXfixes -lssl -lcrypto -lGL"

SOURCES="$SRC_DIR/platform_linux/linux_trader.cpp"

DEBUG_FLAGS="-Wall -Werror -Wextra -Wdouble-promotion -Wformat -Wformat-overflow -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers -Wunused -Wuninitialized -Wno-type-limits -Wno-implicit-fallthrough -Wno-unused-but-set-variable -Og -O0"

OPTIMIZED_FLAGS="-O2"

ASM_FLAVOR="-masm=intel"

if [ ! -d "$BUILD_DIR" ]; then
  mkdir $BUILD_DIR
fi

pushd $BUILD_DIR
g++ $OPTIMIZED_FLAGS -fdiagnostics-show-caret -g -ggdb -gdwarf -g3 -p -fno-exceptions -fno-stack-protector -fno-rtti -save-temps $INCLUDES -nostdinc++ -D_GNU_SOURCE $SOURCES -o $BIN_NAME $LIBRARY_PATHS $LIBRARIES

  chmod +x $BIN_NAME
  cp ../utils/.gdbinit .gdbinit
popd

