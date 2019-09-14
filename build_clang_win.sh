#!/bin/bash
ObjOutDir=build/obj/
BinOutDir=build/

mkdir -p $BinOutDir
mkdir -p $ObjOutDir

#ctime -begin ctime.ctm

CommonDefines="-DAB_PLATFORM_WINDOWS -D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE"
CommonCompilerFlags="-std=c++17 -ffast-math -fno-rtti -fno-exceptions -static-libgcc -static-libstdc++ -fno-strict-aliasing -Werror -Wno-switch"
DebugCompilerFlags="-O0 -fno-inline-functions -g"
ReleaseCompilerFlags="-O2 -finline-functions -g"
PlatformLinkerFlags="-lgdi32 -lopengl32 -luser32 -lWs2_32"
GameLinkerFlags=""

ConfigCompilerFlags=$DebugCompilerFlags

clang++ -save-temps=obj -DAB_PLATFORM_CODE -o $BinOutDir/Soko.exe $CommonDefines $IncludeDirs $CommonCompilerFlags $ConfigCompilerFlags src/Win32Platform.cpp $PlatformLinkerFlags
clang++ -save-temps=obj -o $BinOutDir/SokoGame.dll $CommonDefines $IncludeDirs $CommonCompilerFlags $ConfigCompilerFlags src/Sokoban.cpp -shared  $AppLinkerFlags

#ctime -end ctime.ctm
