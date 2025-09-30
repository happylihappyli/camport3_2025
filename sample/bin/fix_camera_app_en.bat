@echo off
chcp 65001 >nul

REM Check Visual C++ Redistributable installation
ECHO Checking Visual C++ Redistributable installation...
reg query "HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" >nul 2>&1
if %errorlevel% EQU 0 (
    ECHO Visual C++ Redistributable 2015-2022 (x64) is installed
) else (
    ECHO Visual C++ Redistributable 2015-2022 (x64) is NOT installed
    ECHO This is likely the main reason the program cannot run!
    ECHO.
    ECHO Please download and install:
    ECHO https://aka.ms/vs/17/release/vc_redist.x64.exe
)

echo.
REM Check camera connection
ECHO Checking camera connection...
ECHO Please make sure the camera is properly connected to your computer
ECHO and the latest drivers are installed.

echo.
REM Provide administrator run suggestion
ECHO Try running the program as administrator...
ECHO Right-click on PointCloud_v2.exe and select "Run as administrator"

echo.
REM Provide other suggestions
ECHO Other possible solutions:
ECHO 1. Check if there are camera drivers to install in camport3_2025\lib\win\driver directory