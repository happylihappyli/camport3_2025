@echo off
chcp 65001 >nul

:: 设置Visual Studio编译器路径（需要根据实际安装路径调整）
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo 未找到Visual Studio编译器环境变量设置脚本
    echo 请手动设置编译器环境变量或调整本脚本中的路径
    pause
    exit /b 1
)

:: 编译参考实现文件
echo 正在编译funny_mat_reference.cpp...
cl /EHsc funny_mat_reference.cpp /Fe:funny_mat_reference.exe

:: 检查编译结果
if %errorlevel% equ 0 (
    echo 编译成功！生成了funny_mat_reference.exe
    echo 可以通过运行 funny_mat_reference.exe 来测试程序
) else (
    echo 编译失败！请检查错误信息
)

pause