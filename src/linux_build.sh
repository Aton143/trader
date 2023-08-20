#!/bin/bash
echo "Building trader"

if [ -z $NIGHTLY ]; then
  NIGHTLY=0
fi

BUILD_DIR="../build/"
SRC_DIR="../src"

BIN_NAME="trader"

INCLUDES="-I../src/submodules/stb/"

LIBRARY_PATHS=""
LIBRARIES="-lX11 -lXfixes -lGL"

SOURCES="$SRC_DIR/platform_linux/linux_trader.cpp"

DEBUG_FLAGS="-Wall -Werror -Wextra -Wdouble-promotion -Wformat -Wformat-overflow -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers -Wunused -Wuninitialized -Wno-type-limits -Wno-implicit-fallthrough -Wno-unused-but-set-variable -Wno-sizeof-array-div -Wno-write-strings -Og -O0"

OPTIMIZED_FLAGS="-O2"

if [ ! -d "$BUILD_DIR" ]; then
  mkdir $BUILD_DIR
fi

echo "Nightly value: $NIGHTLY"

pushd $BUILD_DIR
  g++ $DEBUG_FLAGS -fdiagnostics-show-caret -g -ggdb -gdwarf -g3 -p -fno-exceptions -fno-stack-protector -fno-rtti -save-temps $INCLUDES -nostdinc++ -D_GNU_SOURCE $SOURCES -o $BIN_NAME $LIBRARY_PATHS $LIBRARIES

  chmod +x $BIN_NAME
  cp ../utils/.gdbinit .gdbinit

  if [ $NIGHTLY -eq 0 ]; then
    glslangValidator $SRC_DIR/platform_linux/*.vert
    glslangValidator $SRC_DIR/platform_linux/*.frag
  fi

popd

exit 0
