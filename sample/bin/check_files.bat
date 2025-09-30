@echo off
chcp 65001 >nul

echo DLL files in bin directory:
dir /b *.dll

echo.
echo Number of executable files in bin directory:
for /f %%i in ('dir /b *.exe ^| find /c "."') do echo %%i

echo.
echo Attempting to run ListDevices_v2.exe...
ListDevices_v2.exe
echo Exit code: %errorlevel%

echo.
echo Possible reasons for failure:
echo 1. Missing Visual C++ Redistributable
echo 2. Incorrect version of tycam.dll
echo 3. Other missing dependencies