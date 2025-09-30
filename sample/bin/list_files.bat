@echo off
chcp 65001 >nul

echo Bin目录中的DLL文件：
dir /b *.dll
echo.
echo Bin目录中的可执行文件数量：
for /f %%i in ('dir /b *.exe ^| find /c "."') do set EXE_COUNT=%%i
echo %EXE_COUNT%
echo.
echo 尝试运行ListDevices_v2.exe（仅输出退出代码）：
ListDevices_v2.exe
set EXIT_CODE=%errorlevel%
echo 退出代码：%EXIT_CODE%
echo.
echo 可能的原因：
echo 1. 缺少Visual C++ Redistributable运行库
if %EXIT_CODE% EQU -1073741515 echo 2. 程序可能需要特定版本的Visual C++ Redistributable（通常是2017或更高版本）
echo 3. 可能缺少其他依赖的DLL文件