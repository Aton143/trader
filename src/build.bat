@echo off

REM MTd: Causes the application to use the multithread, static version of the run-time library. Defines _MT and causes the compiler to place the library name LIBCMT.lib into the .obj file so that the linker will use LIBCMT.lib to resolve external symbols.
REM Gm: Enables (+) / disables (-) minimal rebuild
REM GR: Adds code to check object types at run time (+/-)
REM EH: Specifies the exception handling model support generated by the compiler
REM Od: Turns off all optimizations in the program and speeds compilation.
REM Oi: Replaces some function calls with intrinsic or otherwise special forms of the function that help your application run faster.
REM fp:fast: The /fp:fast option allows the compiler to reorder, combine, or simplify floating-point operations to optimize floating-point code for speed and space.
REM WX: Treats all compiler warnings as errors.
REM W4: All warnings
REM FC: Causes the compiler to display the full path of source code files passed to the compiler in diagnostics.
REM Z7: The /Z7 option produces object files that also contain full symbolic debugging information for use with the debugger
REM W4127: Ignore "conditional expression is constant" warnings

REM incremental:no: Specifies whether to link incrementally or always perform a full link
REM opt:ref: eliminates functions and data that are never referenced

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 64-bit build

del *.pdb > NUL 2> NUL

cl ..\src\platform_win32\win32_trader.cpp /nologo /Od /Oi /FC /Z7  /WX /W4 /wd4201 /MTd /Gm- /GR- /link /incremental:no /opt:ref 

popd



