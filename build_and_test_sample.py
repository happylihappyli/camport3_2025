#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
编译并测试sample程序的数据保存功能
"""

import os
import subprocess
import sys
import shutil
import datetime
import glob

def run_command(cmd, cwd=None):
    """运行命令并返回结果"""
    print(f"执行命令: {cmd}")
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True)
        if result.stdout:
            print("STDOUT:", result.stdout)
        if result.stderr:
            print("STDERR:", result.stderr)
        return result.returncode == 0, result.stdout, result.stderr
    except Exception as e:
        print(f"命令执行失败: {e}")
        return False, "", str(e)

def find_sconstruct():
    """查找SConstruct文件"""
    # 在sample目录及其子目录中查找
    search_paths = [
        "e:/github/camport3/sample",
        "e:/github/camport3/sample/sample_v2",
        "e:/github/camport3/sample/sample_v2/sample",
        "e:/github/camport3/sample/sample_v2/sample/PointCloud"
    ]
    
    for path in search_paths:
        sconstruct_path = os.path.join(path, "SConstruct")
        if os.path.exists(sconstruct_path):
            print(f"找到SConstruct文件: {sconstruct_path}")
            return path
    
    return None

def build_sample():
    """编译sample程序"""
    print("=== 开始编译sample程序 ===")
    
    # 查找SConstruct文件所在目录
    build_dir = find_sconstruct()
    if not build_dir:
        print("错误: 未找到SConstruct文件")
        return False
    
    print(f"编译目录: {build_dir}")
    
    # 尝试使用scons编译
    success, stdout, stderr = run_command("scons", cwd=build_dir)
    if success:
        print("✓ scons编译成功")
        return True
    
    print("scons编译失败，尝试其他方法...")
    
    # 尝试使用make编译
    success, stdout, stderr = run_command("make", cwd=build_dir)
    if success:
        print("✓ make编译成功")
        return True
    
    print("make编译失败，尝试直接编译...")
    
    # 直接编译main.cpp
    pointcloud_dir = "e:/github/camport3/sample/sample_v2/sample/PointCloud"
    output_exe = os.path.join(pointcloud_dir, "main.exe")
    
    # 查找头文件和库文件路径
    include_paths = [
        "e:/github/camport3/include",
        "e:/github/camport3/sample/common",
        "e:/github/camport3/sample/sample_v2/common"
    ]
    
    lib_paths = [
        "e:/github/camport3/lib",
        "e:/github/camport3/lib/x64",
        "e:/github/camport3/sample/lib",
        "e:/github/camport3/sample/sample_v2/lib"
    ]
    
    include_flags = " ".join([f"-I{path}" for path in include_paths if os.path.exists(path)])
    lib_flags = " ".join([f"-L{path}" for path in lib_paths if os.path.exists(path)])
    
    compile_cmd = f"g++ -std=c++11 -o {output_exe} main.cpp {include_flags} {lib_flags} -ltycam -ltycam_api"
    
    success, stdout, stderr = run_command(compile_cmd, cwd=pointcloud_dir)
    if success:
        print("✓ 直接编译成功")
        return True
    
    print("✗ 所有编译方法都失败")
    return False

def test_data_save():
    """测试数据保存功能"""
    print("\n=== 测试数据保存功能 ===")
    
    # 查找可执行文件
    exe_paths = [
        "e:/github/camport3/sample/sample_v2/sample/PointCloud/main.exe",
        "e:/github/camport3/sample/sample_v2/sample/PointCloud/PointCloud.exe",
        "e:/github/camport3/sample/sample_v2/sample/PointCloud/build/main.exe"
    ]
    
    exe_path = None
    for path in exe_paths:
        if os.path.exists(path):
            exe_path = path
            break
    
    if not exe_path:
        print("错误: 未找到可执行文件")
        return False
    
    print(f"使用可执行文件: {exe_path}")
    
    # 获取当前目录
    work_dir = os.path.dirname(exe_path)
    
    # 运行程序
    print("运行sample程序...")
    success, stdout, stderr = run_command(exe_path, cwd=work_dir)
    
    if not success:
        print("✗ 程序运行失败")
        return False
    
    print("✓ 程序运行成功")
    
    # 检查生成的文件
    print("\n检查生成的数据文件:")
    raw_files = glob.glob(os.path.join(work_dir, "*.raw"))
    meta_files = glob.glob(os.path.join(work_dir, "*.meta"))
    ply_files = glob.glob(os.path.join(work_dir, "*.ply"))
    
    print(f"找到 {len(raw_files)} 个原始数据文件:")
    for f in raw_files:
        size = os.path.getsize(f)
        print(f"  {os.path.basename(f)}: {size} 字节")
    
    print(f"找到 {len(meta_files)} 个元数据文件:")
    for f in meta_files:
        print(f"  {os.path.basename(f)}")
        
        # 读取并显示元信息
        try:
            with open(f, 'r') as mf:
                content = mf.read()
                print(f"    内容: {content.strip()}")
        except:
            pass
    
    print(f"找到 {len(ply_files)} 个PLY文件:")
    for f in ply_files:
        size = os.path.getsize(f)
        print(f"  {os.path.basename(f)}: {size} 字节")
    
    # 如果有数据文件，测试成功
    if raw_files or meta_files:
        print("✓ 数据保存功能测试成功")
        return True
    else:
        print("✗ 未找到任何数据文件")
        return False

def copy_test_data():
    """复制测试数据到测试目录"""
    print("\n=== 复制测试数据 ===")
    
    # 创建测试数据目录
    test_dir = "e:/github/camport3_2025/test_data"
    if not os.path.exists(test_dir):
        os.makedirs(test_dir)
    
    # 源目录
    source_dir = "e:/github/camport3/sample/sample_v2/sample/PointCloud"
    
    # 查找数据文件
    raw_files = glob.glob(os.path.join(source_dir, "*.raw"))
    meta_files = glob.glob(os.path.join(source_dir, "*.meta"))
    ply_files = glob.glob(os.path.join(source_dir, "*.ply"))
    
    all_files = raw_files + meta_files + ply_files
    
    if not all_files:
        print("未找到任何数据文件")
        return False
    
    print(f"找到 {len(all_files)} 个数据文件，复制到 {test_dir}")
    
    for file in all_files:
        filename = os.path.basename(file)
        dest_file = os.path.join(test_dir, filename)
        
        try:
            shutil.copy2(file, dest_file)
            print(f"  复制: {filename}")
        except Exception as e:
            print(f"  复制失败 {filename}: {e}")
    
    print("✓ 测试数据复制完成")
    return True

def main():
    """主函数"""
    print("=== Sample程序数据保存功能测试 ===")
    print(f"开始时间: {datetime.datetime.now()}")
    
    # 1. 编译程序
    if not build_sample():
        print("✗ 编译失败，退出测试")
        return False
    
    # 2. 测试数据保存
    if not test_data_save():
        print("✗ 数据保存测试失败")
        return False
    
    # 3. 复制测试数据
    copy_test_data()
    
    print(f"\n=== 测试完成 ===")
    print(f"结束时间: {datetime.datetime.now()}")
    print("✓ 所有测试通过")
    
    return True

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)