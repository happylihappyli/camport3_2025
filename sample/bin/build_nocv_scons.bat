@echo off
chcp 65001 > nul

REM Change working directory to sample folder
echo Changing to sample directory...
cd /d e:\github\camport3_2025\sample

REM Run scons command to build the project
echo Starting to compile PointCloud_NoCV via SConstruct...
echo Compilation may take some time, please wait patiently...
scons

REM Check if compilation was successful
echo.
echo Compilation completed! Checking if PointCloud_NoCV.exe was generated in bin directory
if exist bin\PointCloud_NoCV.exe (
    echo Success: PointCloud_NoCV.exe has been generated!
    echo You can run this OpenCV-independent point cloud program in the bin directory
) else (
    echo Failed: PointCloud_NoCV.exe not found, please check compilation errors
)

REM Display bin directory contents
echo.
echo Bin directory contents:
dir bin\*.exe | findstr "PointCloud"