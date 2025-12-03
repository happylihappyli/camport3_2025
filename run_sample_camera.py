#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
运行sample目录下的相机读取程序并保存数据
用于数据分离测试，区分读取问题和后续处理问题
"""

import os
import subprocess
import shutil
import datetime
import sys

def main():
    print("=" * 60)
    print("运行sample相机读取程序 - 数据分离测试")
    print("=" * 60)
    
    # 路径定义
    sample_exe_path = "e:\\github\\camport3\\sample\\sample_v2\\sample\\PointCloud\\main.exe"
    sample_src_path = "e:\\github\\camport3\\sample\\sample_v2\\sample\\PointCloud\\main.cpp"
    build_dir = "e:\\github\\camport3\\sample\\sample_v2\\sample\\PointCloud"
    output_dir = "e:\\github\\camport3_2025\\test_data"
    
    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    
    # 检查源文件是否存在
    if not os.path.exists(sample_src_path):
        print(f"错误: 找不到源文件 {sample_src_path}")
        print("请确保sample目录存在且包含main.cpp")
        return -1
    
    # 如果可执行文件不存在，尝试编译
    if not os.path.exists(sample_exe_path):
        print("sample可执行文件不存在，尝试编译...")
        print(f"进入目录: {build_dir}")
        
        # 切换到构建目录
        original_cwd = os.getcwd()
        os.chdir(build_dir)
        
        try:
            # 尝试使用scons编译
            if os.path.exists("SConstruct") or os.path.exists("sconstruct"):
                print("发现SConstruct文件，使用scons编译...")
                result = subprocess.run(["scons"], capture_output=True, text=True, encoding='utf-8')
            # 尝试使用make编译
            elif os.path.exists("Makefile") or os.path.exists("makefile"):
                print("发现Makefile文件，使用make编译...")
                result = subprocess.run(["make"], capture_output=True, text=True, encoding='utf-8')
            # 尝试直接编译main.cpp
            else:
                print("尝试使用g++直接编译...")
                compile_cmd = [
                    "g++", "-std=c++11", "-I../../../include", "-L../../../lib",
                    "main.cpp", "-o", "main.exe",
                    "-ltycam", "-lopencv_core", "-lopencv_imgproc", "-lopencv_highgui"
                ]
                result = subprocess.run(compile_cmd, capture_output=True, text=True, encoding='utf-8')
            
            if result.returncode != 0:
                print(f"编译失败! 返回码: {result.returncode}")
                print("标准输出:", result.stdout)
                print("错误输出:", result.stderr)
                return -1
            else:
                print("编译成功!")
                
        except Exception as e:
            print(f"编译过程出错: {e}")
            return -1
        finally:
            os.chdir(original_cwd)
    
    # 再次检查可执行文件
    if not os.path.exists(sample_exe_path):
        print(f"错误: 编译后仍找不到可执行文件 {sample_exe_path}")
        return -1
    
    print(f"找到sample可执行文件: {sample_exe_path}")
    
    # 生成时间戳
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # 运行sample程序获取数据
    print("\n运行sample相机程序获取数据...")
    try:
        # 切换到sample目录运行
        os.chdir(os.path.dirname(sample_exe_path))
        
        result = subprocess.run([sample_exe_path], 
                                capture_output=True, text=True, encoding='utf-8', timeout=30)
        
        print("sample程序运行输出:")
        print("标准输出:", result.stdout)
        if result.stderr:
            print("错误输出:", result.stderr)
            
        if result.returncode != 0:
            print(f"sample程序运行失败，返回码: {result.returncode}")
            return -1
            
    except subprocess.TimeoutExpired:
        print("sample程序运行超时")
        return -1
    except Exception as e:
        print(f"运行sample程序出错: {e}")
        return -1
    finally:
        # 切换回原目录
        os.chdir(original_cwd)
    
    # 查找生成的PLY文件
    print("\n查找生成的PLY文件...")
    ply_files = []
    build_dir_files = os.listdir(build_dir)
    
    for file in build_dir_files:
        if file.endswith('.ply'):
            ply_files.append(os.path.join(build_dir, file))
    
    if not ply_files:
        print("警告: 未找到生成的PLY文件")
        print(f"请检查 {build_dir} 目录")
    else:
        print(f"找到 {len(ply_files)} 个PLY文件:")
        for ply_file in ply_files:
            print(f"  - {ply_file}")
            
            # 复制到测试数据目录
            filename = os.path.basename(ply_file)
            new_filename = f"sample_{timestamp}_{filename}"
            dest_path = os.path.join(output_dir, new_filename)
            
            try:
                shutil.copy2(ply_file, dest_path)
                print(f"    已复制到: {dest_path}")
            except Exception as e:
                print(f"    复制失败: {e}")
    
    # 查找生成的BMP文件（彩色图像）
    print("\n查找生成的BMP文件...")
    bmp_files = []
    
    for file in build_dir_files:
        if file.endswith('.bmp') and 'color' in file:
            bmp_files.append(os.path.join(build_dir, file))
    
    if bmp_files:
        print(f"找到 {len(bmp_files)} 个彩色图像文件:")
        for bmp_file in bmp_files:
            print(f"  - {bmp_file}")
            
            # 复制到测试数据目录
            filename = os.path.basename(bmp_file)
            new_filename = f"sample_{timestamp}_{filename}"
            dest_path = os.path.join(output_dir, new_filename)
            
            try:
                shutil.copy2(bmp_file, dest_path)
                print(f"    已复制到: {dest_path}")
            except Exception as e:
                print(f"    复制失败: {e}")
    
    print("\n" + "=" * 60)
    print("数据获取完成!")
    print(f"测试数据保存在: {output_dir}")
    print("\n下一步操作:")
    print("1. 将获取的数据用于原始处理流程分析")
    print("2. 将获取的数据用于cloud程序处理")
    print("3. 对比两种处理方式的结果，区分问题来源")
    print("=" * 60)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())