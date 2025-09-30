@echo off
chcp 65001 > nul

REM 清理之前可能存在的输出文件
if exist "e:\github\camport3_2025\sample\bin\MinimalTYTest.exe" (
    echo 正在删除旧的 MinimalTYTest.exe 文件...
    del "e:\github\camport3_2025\sample\bin\MinimalTYTest.exe"
)

REM 切换到 sample 目录
cd /d "e:\github\camport3_2025\sample"

REM 使用简化版 SConstruct 文件运行 scons 命令
echo 正在使用简化版 SConstruct_Minimal 编译 MinimalTYTest.cpp...
scons -f SConstruct_Minimal

REM 检查编译结果
if exist "bin\MinimalTYTest.exe" (
    echo 编译成功！MinimalTYTest.exe 已生成
    echo 程序路径：e:\github\camport3_2025\sample\bin\MinimalTYTest.exe
) else (
    echo 编译失败！MinimalTYTest.exe 未生成，请检查错误信息
)

REM 显示 bin 目录下的所有 exe 文件
echo.
echo Bin 目录中的可执行文件：
dir bin\*.exe | findstr "Minimal"
dir bin\*.exe | findstr "PointCloud"