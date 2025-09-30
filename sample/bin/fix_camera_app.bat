@echo off
chcp 65001 >nul

REM 检查Visual C++ Redistributable的安装情况
echo 检查Visual C++ Redistributable安装情况...
reg query "HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" >nul 2>&1
if %errorlevel% EQU 0 (
    echo 已安装 Visual C++ Redistributable 2015-2022 (x64)
) else (
    echo 未安装 Visual C++ Redistributable 2015-2022 (x64)
    echo 这很可能是程序无法运行的主要原因！
    echo.
    echo 请下载并安装：
    echo https://aka.ms/vs/17/release/vc_redist.x64.exe
)

echo.
REM 检查相机连接状态
echo 检查相机连接状态...
echo 请确保相机已正确连接到电脑，并已安装最新的驱动程序。

echo.
REM 提供管理员运行建议
echo 尝试以管理员身份运行程序...
echo 右键点击 PointCloud_v2.exe，选择"以管理员身份运行"

echo.
REM 提供其他建议
echo 其他可能的解决方案：
echo 1. 检查 camport3_2025\lib\win\driver 目录下是否有相机驱动需要安装

echo 按任意键继续...