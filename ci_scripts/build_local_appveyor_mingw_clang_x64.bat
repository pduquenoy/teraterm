setlocal
set COMPILER=mingw_x64
set GENERATOR=Unix Makefiles
set CMAKE_COMMAND=cmake
set CMAKE_OPTION_LIBS=
set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
set CMAKE_OPTION_BUILD=
set MINGW_CC=clang
set MINGW_CXX=clang++
set BUILD_DIR=build_%COMPILER%_msys2_clang_x64
set REV=9999
set DATE_TIME=20200228
set ZIP_FILE=snapshot-r%REV%-%DATE_TIME%-appveyor-%COMPILER%_clang_x64.zip
set SNAPSHOT_DIR=snapshot-r%REV%-%DATE_TIME%-appveyor-%COMPILER%_clang_x64

cd /d %~dp0..
call ci_scripts\build_appveyor.bat