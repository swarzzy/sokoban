@echo off
set ExecArg=run
if "%1" == "%ExecArg%" (
pushd build
Soko.exe
popd
goto end
)

set ObjOutDir=build\obj\
set BinOutDir=build\

IF NOT EXIST %BinOutDir% mkdir %BinOutDir%
IF NOT EXIST %ObjOutDir% mkdir %ObjOutDir%

rem cls

ctime -begin ctime.ctm
del %BinOutDir%*.pdb >NUL 2>&1
set PdbMangleVal=%date:~6,4%%date:~3,2%%date:~0,2%%time:~1,1%%time:~3,2%%time:~6,2%

set CommonDefines=/D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DAB_PLATFORM_WINDOWS /DUNICODE /D_UNICODE /DUNITY_BUILD
set CommonCompilerFlags=/Gm- /fp:fast /GR- /nologo /diagnostics:classic /WX /std:c++17
set DebugCompilerFlags=/Zi /Od /RTC1 /MTd /Fd%BinOutDir% /DSOKO_DEBUG
set ReleaseCompilerFlags=/O2 /MT /Oi /MT /Zi
set PlatformLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /NOIMPLIB user32.lib gdi32.lib opengl32.lib winmm.lib /OUT:%BinOutDir%\Soko.exe /PDB:%BinOutDir%\Soko.pdb
set GameLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /DLL /OUT:%BinOutDir%\SokoGame.dll Ws2_32.lib /PDB:%BinOutDir%\SokoGame_%PdbMangleVal%.pdb
rem set RendererLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /DLL /OUT:%BinOutDir%\SokoRenderer.dll /PDB:%BinOutDir%\SokoRenderer_%PdbMangleVal%.pdb

set ConfigCompilerFlags=%DebugCompilerFlags%

set PrepBuildArg=prepbuild
if "%1" == "%PrepBuildArg%" (
cl /W3 /Fo%ObjOutDir% /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN %ReleaseCompilerFlags% %ConfigCompilerFlags% src/Preprocessor.cpp /link /INCREMENTAL:NO /OPT:REF /MACHINE:X64 /OUT:%BinOutDir%\Prep.exe /PDB:%BinOutDir%\Prep.pdb
goto end
)

set AssetBuildArg=assetbuild
if "%1" == "%PrepBuildArg%" (
cl /W3 /Fo%ObjOutDir% /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN %CommonCompilerFlags% %ConfigCompilerFlags% src/Preprocessor.cpp /link /INCREMENTAL:NO /OPT:REF /MACHINE:X64 /OUT:%BinOutDir%\AssetBuilder.exe /PDB:%BinOutDir%\AssetBuilder.pdb
goto end
)

rem echo Building preprocessor...
rem cl /W3 /Fo%ObjOutDir% /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN %CommonCompilerFlags% %ConfigCompilerFlags% src/Preprocessor.cpp /link /INCREMENTAL:NO /OPT:REF /MACHINE:X64 /OUT:%BinOutDir%\Prep.exe /PDB:%BinOutDir%\Prep.pdb

rem echo Building asset builder...
rem cl /W3 /Fo%ObjOutDir% /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN %CommonCompilerFlags% %ConfigCompilerFlags% src/AssetBuilder.cpp /link /INCREMENTAL:NO /OPT:REF /MACHINE:X64 /OUT:%BinOutDir%\AssetBuilder.exe /PDB:%BinOutDir%\AssetBuilder.pdb

rem echo Building cubemap builder...
rem cl /W3 /Fo%ObjOutDir% /D_CRT_SECURE_NO_WARNINGS  %CommonCompilerFlags% %ConfigCompilerFlags% src/CubemapBuilder.cpp /link /INCREMENTAL:NO /OPT:REF /MACHINE:X64 /OUT:%BinOutDir%\CubemapBuilder.exe /PDB:%BinOutDir%\CubemapBuilder.pdb

echo Building shader preprocessor...
cl /W3 /wd4530 /Gm- /GR- /O2 /MT /nologo /diagnostics:classic /WX /std:c++17 /Fo%ObjOutDir% /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN  src/tools/ShaderPreprocessor.cpp /link /INCREMENTAL:NO /OPT:REF /MACHINE:X64 /OUT:%BinOutDir%\ShaderPreprocessor.exe /PDB:%BinOutDir%\ShaderPreprocessor.pdb

echo Preprocessing shaders...
pushd src
..\build\ShaderPreprocessor.exe ShaderConfig.txt
popd


echo Generating meta info...
build\Prep.exe MetaInfo_Generated.h src/MetaInfo_Generated src/Entity.h src/Level.h src/Renderer.cpp src/EntityBehavior.h src/Chunk.h
rem src/Entity.h src/Level.h src/Platform.h

echo Building platform...
start /b "__soko_compilation__" cmd /c cl /W3 /FS /DAB_PLATFORM_CODE /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Win32Platform.cpp /link %PlatformLinkerFlags%
rem cl /MP /W3 /FS /DAB_PLATFORM_CODE /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Win32Platform.cpp /link %PlatformLinkerFlags%

echo Building game...
start /b /wait "__soko_compilation__" cmd /c cl /W3 /FS /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Sokoban.cpp /link %GameLinkerFlags%
rem cl /MP /W3 /FS /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Sokoban.cpp /link %GameLinkerFlags%

ctime -end ctime.ctm
:end
