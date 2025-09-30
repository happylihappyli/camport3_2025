# PointCloud_NoCV 程序说明

这是一个不依赖OpenCV的点云采集程序，专为无法或不想安装OpenCV的环境设计。

## 功能特点
- 完全不依赖OpenCV库
- 可以连接相机设备并获取深度图像
- 支持获取彩色图像（如果相机支持）
- 将点云数据保存为PLY文件格式
- 简单易用的命令行界面

## 使用方法

### 方法一：直接运行（如果已编译）

如果PointCloud_NoCV.exe已存在于当前目录，您可以直接运行：

```
PointCloud_NoCV.exe
```

### 方法二：编译程序

如果需要自己编译程序，请按照以下步骤操作：

1. 确保您的系统已安装Visual Studio（推荐2017或更高版本）
2. 使用Visual Studio提供的"x64 Native Tools Command Prompt"（64位本机工具命令提示符）
3. 导航到当前目录（camport3_2025\sample\bin）
4. 运行编译脚本：

```
compile_nocv.bat
```

5. 编译成功后，将生成PointCloud_NoCV.exe文件

## 程序运行流程

1. 程序启动后，会自动搜索并连接第一个可用的相机设备
2. 打开深度流（如果相机支持，还会尝试打开RGB流）
3. 获取一帧图像数据（等待最多2000毫秒）
4. 处理深度图像数据，生成点云
5. 将点云数据保存为PLY文件（文件名包含时间戳）
6. 自动退出程序

## 输出文件

程序会在当前目录生成以"point_cloud_"开头的PLY文件，文件名包含时间戳信息。

您可以使用常见的3D查看器（如MeshLab、CloudCompare等）打开和查看这些PLY文件。

## 注意事项

- 确保tycam.dll文件在系统PATH环境变量中，或复制到与可执行文件相同的目录
- 如果程序无法找到相机设备，请检查相机连接和驱动安装情况
- 如果需要连续采集多帧点云数据，需要修改源代码并重新编译

## 故障排除

1. **找不到MSVC编译器**：
   - 请使用Visual Studio提供的"x64 Native Tools Command Prompt"运行编译脚本

2. **找不到tycam.dll**：
   - 将tycam.dll所在目录添加到系统PATH环境变量
   - 或直接将tycam.dll复制到与PointCloud_NoCV.exe相同的目录

3. **无法连接相机**：
   - 检查相机USB/网络连接
   - 确保已安装相机驱动程序
   - 尝试以管理员身份运行程序

如果您遇到其他问题，可以参考camport3_2025文档或联系技术支持。