#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
数据分离测试主脚本
运行流程：
1. 运行sample相机程序获取原始数据
2. 分析原始数据的质量
3. 对比cloud程序的处理结果
4. 生成测试报告
"""

import os
import sys
import subprocess
import datetime

def run_script(script_path, description):
    """运行指定脚本"""
    print(f"\n{'='*60}")
    print(f"开始: {description}")
    print(f"脚本: {script_path}")
    print('='*60)
    
    if not os.path.exists(script_path):
        print(f"错误: 找不到脚本 {script_path}")
        return False
    
    try:
        result = subprocess.run([sys.executable, script_path], 
                                capture_output=True, text=True, encoding='utf-8')
        
        print("运行输出:")
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print("错误信息:")
            print(result.stderr)
        
        success = result.returncode == 0
        if success:
            print(f"✓ {description} - 成功")
        else:
            print(f"✗ {description} - 失败 (返回码: {result.returncode})")
        
        return success
        
    except Exception as e:
        print(f"运行脚本出错: {e}")
        return False

def generate_test_report():
    """生成测试报告"""
    report_path = "e:\\github\\camport3_2025\\test_report.txt"
    test_data_dir = "e:\\github\\camport3_2025\\test_data"
    
    print(f"\n生成测试报告: {report_path}")
    
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write("相机数据处理分离测试报告\n")
        f.write("=" * 50 + "\n")
        f.write(f"测试时间: {datetime.datetime.now()}\n")
        f.write(f"测试数据目录: {test_data_dir}\n\n")
        
        # 检查测试数据
        if os.path.exists(test_data_dir):
            files = os.listdir(test_data_dir)
            ply_files = [f for f in files if f.endswith('.ply')]
            bmp_files = [f for f in files if f.endswith('.bmp')]
            
            f.write("数据文件统计:\n")
            f.write(f"  PLY文件数量: {len(ply_files)}\n")
            f.write(f"  BMP文件数量: {len(bmp_files)}\n\n")
            
            if ply_files:
                f.write("PLY文件列表:\n")
                for ply_file in ply_files:
                    f.write(f"  - {ply_file}\n")
                f.write("\n")
            
            if bmp_files:
                f.write("BMP文件列表:\n")
                for bmp_file in bmp_files:
                    f.write(f"  - {bmp_file}\n")
                f.write("\n")
        else:
            f.write("测试数据目录不存在\n\n")
        
        f.write("测试结论:\n")
        f.write("1. 如果sample程序生成的PLY文件包含正常的彩色点云数据\n")
        f.write("2. 而cloud程序处理后彩色数据丢失或异常\n")
        f.write("3. 则可以确定问题在cloud程序的后续处理流程\n")
        f.write("4. 需要重点检查cloud程序的颜色处理逻辑\n\n")
        
        f.write("建议检查项:\n")
        f.write("- sample程序生成的PLY文件是否包含颜色信息\n")
        f.write("- cloud程序的颜色处理算法是否正确\n")
        f.write("- 像素格式转换是否有问题\n")
        f.write("- 点云数据处理流程是否丢失颜色\n")
    
    print(f"测试报告已生成: {report_path}")

def main():
    print("=" * 60)
    print("数据分离测试主程序")
    print("用于区分相机读取问题和后续处理问题")
    print("=" * 60)
    
    # 步骤1: 运行sample相机程序获取数据
    success1 = run_script(
        "e:\\github\\camport3_2025\\run_sample_camera.py",
        "运行sample相机程序获取原始数据"
    )
    
    if not success1:
        print("\n✗ 第一步失败，停止测试")
        return -1
    
    # 步骤2: 使用cloud程序分析数据
    success2 = run_script(
        "e:\\github\\camport3_2025\\process_with_cloud.py", 
        "使用cloud程序分析PLY数据"
    )
    
    # 生成测试报告
    generate_test_report()
    
    print("\n" + "=" * 60)
    print("测试完成总结:")
    print("=" * 60)
    
    if success1 and success2:
        print("✓ 所有测试步骤完成")
        print("✓ 请查看测试报告和数据文件")
        print("✓ 对比分析结果以确定问题来源")
    elif success1:
        print("✓ 数据获取成功")
        print("✗ cloud程序分析失败")
        print("- 请检查cloud程序配置")
    else:
        print("✗ 数据获取失败")
        print("- 请检查sample程序和相机连接")
    
    print(f"\n测试数据位置: e:\\github\\camport3_2025\\test_data")
    print(f"测试报告位置: e:\\github\\camport3_2025\\test_report.txt")
    print("=" * 60)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())