@echo off

IF NOT EXIST build mkdir build

pushd build

cl ..\main.cpp /nologo /Od /Zi /wd4312

popd

