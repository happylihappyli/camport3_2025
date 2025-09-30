@echo off
chcp 65001 >nul

:: 检查g++是否可用
g++ --version >nul 2>&1
if %errorlevel% neq 0 (
    echo 未找到g++编译器
    echo 请先安装MinGW并将其添加到系统PATH环境变量中
    pause
    exit /b 1
)

:: 使用g++编译参考实现文件
echo 正在使用g++编译funny_mat_reference.cpp...
g++ -std=c++11 funny_mat_reference.cpp -o funny_mat_reference_mingw.exe

:: 检查编译结果
if %errorlevel% equ 0 (
    echo 编译成功！生成了funny_mat_reference_mingw.exe
    echo 可以通过运行 funny_mat_reference_mingw.exe 来测试程序
) else (
    echo 编译失败！请检查错误信息
)

pause