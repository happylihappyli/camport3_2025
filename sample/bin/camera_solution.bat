@echo off
chcp 65001 >nul

echo ===== SOLUTION FOR CAMERA APPLICATION =====
echo.
echo 1. INSTALL VISUAL C++ REDISTRIBUTABLE
echo =================================
echo This is the most likely reason the program cannot run!
echo Download and install from:
echo https://aka.ms/vs/17/release/vc_redist.x64.exe
echo.
echo 2. CHECK CAMERA CONNECTION
echo =========================
echo - Make sure the camera is properly connected to your PC
echo - Check if camera drivers are installed

echo.
echo 3. RUN AS ADMINISTRATOR
echo ======================
echo Right-click on PointCloud_v2.exe and select "Run as administrator"

echo.
echo 4. CHECK DRIVER FOLDER
echo =====================
echo Look for driver installation files in:
echo camport3_2025\lib\win\driver