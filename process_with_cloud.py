#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
使用cloud程序处理PLY文件的脚本
用于数据分离测试，分析后续处理问题
"""

import os
import subprocess
import shutil
import sys
import glob

def process_with_cloud_program(ply_file_path):
    """使用cloud程序处理PLY文件"""
    print(f"\n使用cloud程序处理: {ply_file_path}")
    
    # cloud程序路径
    cloud_exe = "e:\\github\\camport3_2025\\cloud\\cloud_point_cloud.exe"
    
    if not os.path.exists(cloud_exe):
        print(f"错误: 找不到cloud程序 {cloud_exe}")
        print("请先编译cloud程序")
        return False
    
    # 获取文件名和目录
    filename = os.path.basename(ply_file_path)
    file_dir = os.path.dirname(ply_file_path)
    
    # 生成输出文件名
    name_without_ext = os.path.splitext(filename)[0]
    output_filename = f"cloud_processed_{name_without_ext}.ply"
    output_path = os.path.join(file_dir, output_filename)
    
    print(f"输入文件: {ply_file_path}")
    print(f"预期输出: {output_path}")
    
    try:
        # 切换到cloud目录运行
        cloud_dir = os.path.dirname(cloud_exe)
        original_cwd = os.getcwd()
        os.chdir(cloud_dir)
        
        # 运行cloud程序
        # 注意: cloud程序目前设计为从相机读取数据，需要修改才能处理文件
        print("注意: cloud程序目前设计为从相机读取数据")
        print("需要修改程序才能直接处理PLY文件")
        
        # 这里我们暂时只是复制文件作为示例
        print("暂时复制文件作为处理结果...")
        shutil.copy2(ply_file_path, output_path)
        
        os.chdir(original_cwd)
        
        if os.path.exists(output_path):
            print(f"处理完成，输出文件: {output_path}")
            return True
        else:
            print("处理失败: 未生成输出文件")
            return False
            
    except Exception as e:
        print(f"处理过程出错: {e}")
        return False

def analyze_ply_file(ply_file_path):
    """分析PLY文件内容"""
    print(f"\n分析PLY文件: {ply_file_path}")
    
    try:
        with open(ply_file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        # 统计顶点数量
        vertex_count = 0
        header_end = False
        valid_points = 0
        colorless_points = 0
        
        for i, line in enumerate(lines):
            line = line.strip()
            
            if not header_end:
                if line == "end_header":
                    header_end = True
                    print(f"头部信息结束于第{i+1}行")
                elif line.startswith("element vertex"):
                    vertex_count = int(line.split()[2])
                    print(f"顶点数量: {vertex_count}")
                continue
            
            # 分析数据部分
            if line and not line.startswith("#"):
                parts = line.split()
                if len(parts) >= 3:  # 至少有x,y,z坐标
                    if len(parts) >= 6:  # 有颜色信息 (r,g,b)
                        try:
                            # 检查颜色值是否有效
                            r, g, b = float(parts[3]), float(parts[4]), float(parts[5])
                            if r > 0 or g > 0 or b > 0:
                                valid_points += 1
                            else:
                                colorless_points += 1
                        except:
                            colorless_points += 1
                    else:
                        colorless_points += 1
        
        print(f"有效彩色点: {valid_points}")
        print(f"无彩色点: {colorless_points}")
        print(f"总数据行数: {len([l for l in lines if l.strip() and not l.startswith('#')]) - 1}")  # 减去end_header
        
        return {
            'vertex_count': vertex_count,
            'valid_points': valid_points,
            'colorless_points': colorless_points,
            'total_lines': len(lines)
        }
        
    except Exception as e:
        print(f"分析文件出错: {e}")
        return None

def main():
    print("=" * 60)
    print("使用cloud程序处理PLY文件 - 数据分离测试")
    print("=" * 60)
    
    # 测试数据目录
    test_data_dir = "e:\\github\\camport3_2025\\test_data"
    
    if not os.path.exists(test_data_dir):
        print(f"错误: 测试数据目录不存在 {test_data_dir}")
        print("请先运行 run_sample_camera.py 获取数据")
        return -1
    
    # 查找PLY文件
    ply_files = glob.glob(os.path.join(test_data_dir, "*.ply"))
    
    if not ply_files:
        print(f"在 {test_data_dir} 中未找到PLY文件")
        return -1
    
    print(f"找到 {len(ply_files)} 个PLY文件:")
    for i, ply_file in enumerate(ply_files):
        print(f"  {i+1}. {os.path.basename(ply_file)}")
    
    # 分析每个PLY文件
    analysis_results = {}
    for ply_file in ply_files:
        result = analyze_ply_file(ply_file)
        if result:
            analysis_results[ply_file] = result
    
    # 显示分析总结
    print("\n" + "=" * 60)
    print("PLY文件分析总结:")
    print("=" * 60)
    
    for ply_file, result in analysis_results.items():
        filename = os.path.basename(ply_file)
        print(f"\n文件: {filename}")
        print(f"  声明顶点数: {result['vertex_count']}")
        print(f"  有效彩色点: {result['valid_points']}")
        print(f"  无彩色点: {result['colorless_points']}")
        print(f"  彩色点比例: {result['valid_points']/max(result['vertex_count'],1)*100:.1f}%")
    
    print("\n" + "=" * 60)
    print("处理建议:")
    print("1. 如果sample程序生成的PLY文件彩色点比例正常")
    print("2. 但cloud程序处理后彩色点丢失或异常")
    print("3. 说明问题在cloud程序的后续处理流程")
    print("4. 需要检查cloud程序的颜色处理逻辑")
    print("=" * 60)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())