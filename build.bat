@echo off

cls

REM https://stackoverflow.com/a/29379089/4894526
set SystemLibs=opengl32.lib user32.lib gdi32.lib shell32.lib 

REM https://stackoverflow.com/a/78289129/4894526
set Libs=deps\glew32s.lib deps\glfw3_mt.lib 

set Flags=/nologo /Wall /Zi /Qspectre

cl /Fe:"bin\game" /Fo:"bin\game" /Ideps %Flags% %SystemLibs% %Libs% src\main.c 

if %errorlevel% neq 0 (
    echo.
    echo ***Build failed***
    exit /B 1
)

if not "%1" == "-b" ( REM Build only switch
    bin\game.exe
)
