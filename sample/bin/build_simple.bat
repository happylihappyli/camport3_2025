@echo off

REM Use UTF-8 encoding
chcp 65001 > nul

REM Clean up old files
if exist "e:\github\camport3_2025\sample\bin\MinimalTYTest.exe" (
    del "e:\github\camport3_2025\sample\bin\MinimalTYTest.exe"
)

REM Change to sample directory
cd /d "e:\github\camport3_2025\sample"

REM Run scons with the minimal SConstruct file
scons -f SConstruct_Minimal

REM Check if compilation was successful
if exist "bin\MinimalTYTest.exe" (
    echo Success: MinimalTYTest.exe has been generated
) else (
    echo Error: MinimalTYTest.exe not generated
)

REM List all exe files in bin directory
cd bin
dir *.exe