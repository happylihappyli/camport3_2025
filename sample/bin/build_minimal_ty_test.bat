@echo off
chcp 65001 > nul

REM 清理之前的编译结果
if exist MinimalTYTest.obj del MinimalTYTest.obj
if exist MinimalTYTest.exe del MinimalTYTest.exe

REM 设置编译环境
set "VSINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community"
set "VCINSTALLDIR=%VSINSTALLDIR%\VC"
set "MSVCDIR=%VCINSTALLDIR%\Tools\MSVC\14.44.35207"
set "PATH=%MSVCDIR%\bin\Hostx64\x64;%PATH%"
set "INCLUDE=%MSVCDIR%\include;E:\github\camport3_2025\include;%INCLUDE%"
set "LIB=%MSVCDIR%\lib\x64;E:\github\camport3_2025\lib\x64;%LIB%"

REM 编译MinimalTYTest.cpp文件
cl /EHsc /TP /nologo /D_CRT_SECURE_NO_WARNINGS /I "E:\github\camport3_2025\include" MinimalTYTest.cpp /link /LIBPATH:"E:\github\camport3_2025\lib\x64" tycam.lib ws2_32.lib user32.lib /OUT:MinimalTYTest.exe

REM 检查编译结果
if exist MinimalTYTest.exe (
    echo 编译成功！MinimalTYTest.exe 已生成
) else (
    echo 编译失败！请检查错误信息
)

REM 显示bin目录下的所有exe文件
dir *.exe