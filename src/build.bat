@echo off

set INCLUDE_DIRS=
set INCLUDE_DIRS=%INCLUDE_DIRS% /I D:\openssl\include\

set LIBS=
set LIBS=%LIBS% D:\openssl\libssl.lib
set LIBS=%LIBS% D:\openssl\libcrypto.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL

cl ..\src\platform_win32\win32_trader.cpp %INCLUDE_DIRS% /nologo /Od /Oi /FC /Z7 /WX /W4 /MT /Gm- /GR- /wd4201 /wd4706 /EHa- /EHc- /EHsc- /wd4505 /link %LIBS% /incremental:no /opt:ref

popd
