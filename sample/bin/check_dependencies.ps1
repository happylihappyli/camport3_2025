# 检查当前目录中的DLL文件
Write-Host "当前目录中的DLL文件："
Get-ChildItem -Filter '*.dll' | Format-Table -Property Name, Length, LastWriteTime

# 列出可执行文件信息
Write-Host "\nPointCloud_v2.exe文件信息："
Get-ChildItem -Path .\PointCloud_v2.exe | Format-List -Property *

# 检查系统是否有Visual C++ Redistributable
Write-Host "\n检查Visual C++ Redistributable是否已安装："
try {
    Get-WmiObject -Class Win32_Product | Where-Object {$_.Name -like "*Visual C++*"} | Select-Object Name, Version
} catch {
    Write-Host "无法查询已安装的程序。可能需要管理员权限。"
}

Write-Host "\n建议："
Write-Host "1. 确保已安装Visual C++ Redistributable 2017或更高版本"
Write-Host "2. 检查是否有缺少的DLL文件"
Write-Host "3. 使用Dependency Walker或Process Explorer等工具进一步分析依赖项问题"