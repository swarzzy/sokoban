@echo off
set ExecArg=exec
if "%1" == "%ExecArg%" (
pushd build
start "Sokoban" Soko.exe
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

set CommonDefines=/D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DAB_PLATFORM_WINDOWS /DUNICODE /D_UNICODE
set CommonCompilerFlags=/Gm- /fp:fast /GR- /nologo /diagnostics:classic /WX /std:c++17
set DebugCompilerFlags=/Zi /Od /RTC1 /Ob0 /MTd /Fd%BinOutDir% /DSOKO_DEBUG
set ReleaseCompilerFlags=/Ox /Ob2 /MT /Oi /MT /Zi
set PlatformLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 Ws2_32.lib user32.lib gdi32.lib opengl32.lib /OUT:%BinOutDir%\Soko.exe /PDB:%BinOutDir%\Soko.pdb
set GameLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /DLL /OUT:%BinOutDir%\SokoGame.dll /PDB:%BinOutDir%\SokoGame_%PdbMangleVal%.pdb
rem set RendererLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /DLL /OUT:%BinOutDir%\SokoRenderer.dll /PDB:%BinOutDir%\SokoRenderer_%PdbMangleVal%.pdb

set ConfigCompilerFlags=%DebugCompilerFlags%
echo Building platform...
start /b "__soko_compilation__" cmd /c cl /W3 /FS /DAB_PLATFORM_CODE /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Win32Platform.cpp /link %PlatformLinkerFlags%
rem cl /MP /W3 /FS /DAB_PLATFORM_CODE /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Win32Platform.cpp /link %PlatformLinkerFlags%

echo Building game...
start /b /wait "__soko_compilation__" cmd /c cl /W3 /FS /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Sokoban.cpp /link %GameLinkerFlags%
rem cl /MP /W3 /FS /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Sokoban.cpp /link %GameLinkerFlags%

rem echo Building renderer...
rem cl /MP /W3 /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/Renderer.cpp /link %RendererLinkerFlags%
rem set RendererResult=%ERRORLEVEL%

set BuildArg=run

if "%1" == "%BuildArg%" (
pushd build
start "Sokoban" Soko.exe
popd
)
ctime -end ctime.ctm
:end
