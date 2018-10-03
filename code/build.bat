@echo off

set CommonCompilerFlags=-MT -nologo -EHa- -WX -W4 -fp:fast -Od -Oi -FC -Z7 -wd4100 -wd4127 -wd4201 -wd4189 -wd4459 -wd4505
set CommonCompilerFlags=-D_HAS_EXCEPTIONS=0 -D_CRT_SECURE_NO_WARNINGS %CommonCompilerFlags%
set CommonLinkerFlags=-incremental:no  user32.lib gdi32.lib winmm.lib opengl32.lib SDL2.lib SDL2main.lib freetype.lib
set CommonIncludeFlags=-I ..\deps\SDL\include -I ..\deps\gl3w\include -I ..\deps\freetype\include -I ..\code

set AddLibsx64=-LIBPATH:..\deps\SDL\VisualC\x64\Release -LIBPATH:..\deps\freetype\lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl %CommonCompilerFlags% %CommonIncludeFlags% ..\code\main.cpp /link -subsystem:windows %AddLibsx64% %AddLibsx64% %CommonLinkerFlags%

popd