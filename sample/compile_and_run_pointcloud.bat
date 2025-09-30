@echo off
chcp 65001 >nul

REM 设置编译器和链接器选项
set COMPILER=g++
set CPP_FILES=PointCloudReader.cpp PointCloudReader_test.cpp
set INCLUDE_PATH=-I../include
set LIB_PATH=-L../lib
set LIBS=-lTYSdk
set OUTPUT=PointCloudReader.exe

REM 检查编译器是否可用
where %COMPILER% >nul 2>nul
if %errorlevel% neq 0 (
echo 错误: 未找到C++编译器，请确保已安装MinGW或Visual Studio并配置了环境变量。
pause
 exit /b 1
)

REM 编译程序
echo 正在编译程序...
%COMPILER% %CPP_FILES% %INCLUDE_PATH% %LIB_PATH% %LIBS% -o %OUTPUT%

if %errorlevel% neq 0 (
echo 编译失败！
pause
 exit /b 1
)

REM 运行程序
echo 编译成功，正在运行程序...
%OUTPUT%

if %errorlevel% neq 0 (
echo 程序运行出错！
pause
 exit /b 1
)

REM 清理临时文件
echo 程序执行完毕！
pause