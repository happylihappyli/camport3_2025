@echo off
chcp 65001 >nul

echo Checking tycam.dll information...
rem Try to get file version information
for %%I in (tycam.dll) do (
    echo File name: %%~nxI
    echo File size: %%~zI bytes
)

echo.
echo Possible missing components:
echo 1. Visual C++ Redistributable (required for most C++ applications)
echo 2. Other runtime libraries specific to the camera SDK

echo.
echo Suggestions to fix the issue:
echo 1. Install Visual C++ Redistributable 2017 or later
echo 2. Check if the camera is properly connected
echo 3. Contact the camera manufacturer for SDK documentation
echo 4. Run the application as administrator