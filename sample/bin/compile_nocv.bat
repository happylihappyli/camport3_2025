@echo off
chcp 65001 >nul

setlocal enabledelayedexpansion

REM 编译不依赖OpenCV的PointCloud_NoCV程序
REM 这个脚本使用MSVC直接编译，不依赖CMake构建系统

REM 设置项目路径
set PROJECT_ROOT=..\..
set SOURCE_FILE=%PROJECT_ROOT%\sample\sample_v2\sample\PointCloud\PointCloud_NoCV.cpp
set OUTPUT_DIR=%PROJECT_ROOT%\sample\bin
set OUTPUT_EXE=%OUTPUT_DIR%\PointCloud_NoCV.exe

REM 设置头文件路径
set INCLUDE_PATHS=/I"%PROJECT_ROOT%\include" /I"%PROJECT_ROOT%\sample\sample_v2\hpp" /I"%PROJECT_ROOT%\sample\common"

REM 设置库文件路径
set LIB_PATHS=/link /LIBPATH:"%PROJECT_ROOT%\sample\lib\win\hostapp\x64"

REM 设置链接的库
set LIBS=tycam.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

REM 检查是否存在MSVC编译器
where cl >nul 2>nul
if %errorlevel% neq 0 (
echo 错误：未找到MSVC编译器。
echo 请使用Visual Studio提供的"x64 Native Tools Command Prompt"运行此脚本。
echo 或者确保已安装Visual Studio并添加了MSVC到环境变量。
pause
 exit /b 1
)

REM 检查源文件是否存在
if not exist "%SOURCE_FILE%" (
echo 错误：未找到源文件 %SOURCE_FILE%
echo 请确保PointCloud_NoCV.cpp文件已正确创建。
pause
 exit /b 1
)

REM 确保输出目录存在
if not exist "%OUTPUT_DIR%" (
mkdir "%OUTPUT_DIR%"
 if %errorlevel% neq 0 (
echo 错误：无法创建输出目录 %OUTPUT_DIR%
pause
  exit /b 1
 )
echo 创建输出目录：%OUTPUT_DIR%
)

REM 编译命令
set COMPILE_CMD=cl /EHsc /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /W3 /WX- /nologo /FD /c %INCLUDE_PATHS% "%SOURCE_FILE%"

REM 链接命令
set LINK_CMD=link /OUT:"%OUTPUT_EXE%" /NOLOGO %LIB_PATHS% %LIBS%

REM 执行编译
cd /d "%OUTPUT_DIR%"
echo 正在编译PointCloud_NoCV.cpp...
echo 编译命令: %COMPILE_CMD%
%COMPILE_CMD%
if %errorlevel% neq 0 (
echo 错误：编译失败。请查看上面的错误信息。
pause
 exit /b 1
)

REM 执行链接
echo 正在链接PointCloud_NoCV.exe...
echo 链接命令: %LINK_CMD%
%LINK_CMD%
if %errorlevel% neq 0 (
echo 错误：链接失败。请查看上面的错误信息。

REM 清理中间文件
del "%OUTPUT_DIR%\PointCloud_NoCV.obj" >nul 2>nul

pause
 exit /b 1
)

REM 清理中间文件
del "%OUTPUT_DIR%\PointCloud_NoCV.obj" >nul 2>nul

REM 检查是否生成了可执行文件
if exist "%OUTPUT_EXE%" (
echo.
echo ======== 编译完成 ========
echo 成功生成不依赖OpenCV的点云程序：
echo %OUTPUT_EXE%
echo.
echo 您可以直接运行此程序，无需安装OpenCV。
echo 程序功能：
echo 1. 连接相机设备
2. 获取深度图像（和彩色图像，如果有）
3. 生成点云数据并保存为PLY文件
4. 自动退出程序
echo =========================
) else (
echo 错误：未找到生成的PointCloud_NoCV.exe文件。
pause
 exit /b 1
)

endlocal