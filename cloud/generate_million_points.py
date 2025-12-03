#!/usr/bin/env python3
"""
生成百万点云PLY文件
生成包含1,000,000个点的点云文件，用于测试不同软件的点云处理能力
"""

import numpy as np
import random
import time
import os
from datetime import datetime

def generate_point_cloud_ply(num_points=1000000, output_file="million_points.ply", 
                            distribution="random", add_color=True):
    """
    生成PLY格式的点云文件
    
    参数:
        num_points: 点的数量（默认100万）
        output_file: 输出文件名
        distribution: 点云分布类型
            - "random": 随机分布
            - "sphere": 球体分布
            - "cube": 立方体分布
            - "plane": 平面分布
        add_color: 是否添加颜色信息
    """
    print(f"开始生成 {num_points:,} 个点的点云文件...")
    start_time = time.time()
    
    # 生成点云数据
    if distribution == "random":
        # 随机分布：在 -1000 到 1000 的范围内
        points = np.random.uniform(-1000, 1000, size=(num_points, 3))
    elif distribution == "sphere":
        # 球体分布：半径1000的球体
        phi = np.random.uniform(0, 2 * np.pi, num_points)
        costheta = np.random.uniform(-1, 1, num_points)
        theta = np.arccos(costheta)
        r = np.random.uniform(0, 1000, num_points)
        points = np.zeros((num_points, 3))
        points[:, 0] = r * np.sin(theta) * np.cos(phi)
        points[:, 1] = r * np.sin(theta) * np.sin(phi)
        points[:, 2] = r * np.cos(theta)
    elif distribution == "cube":
        # 立方体分布：边长为2000的立方体
        points = np.random.uniform(-1000, 1000, size=(num_points, 3))
    elif distribution == "plane":
        # 平面分布：Z=0的平面上
        points = np.random.uniform(-1000, 1000, size=(num_points, 3))
        points[:, 2] = 0  # Z坐标设为0
    else:
        raise ValueError(f"未知的分布类型: {distribution}")
    
    # 生成颜色（可选）
    colors = None
    if add_color:
        if distribution == "sphere":
            # 球体：根据距离中心的距离生成渐变色
            distances = np.linalg.norm(points, axis=1)
            normalized_dist = distances / distances.max()
            colors = np.zeros((num_points, 3), dtype=np.uint8)
            colors[:, 0] = (normalized_dist * 255).astype(np.uint8)  # Red
            colors[:, 1] = ((1 - normalized_dist) * 255).astype(np.uint8)  # Green
            colors[:, 2] = 128  # Blue
        else:
            # 随机颜色
            colors = np.random.randint(0, 256, size=(num_points, 3), dtype=np.uint8)
    
    # 写入PLY文件
    print(f"正在写入文件: {output_file}")
    with open(output_file, 'w') as f:
        # 写入PLY文件头
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {num_points}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")
        if add_color and colors is not None:
            f.write("property uchar red\n")
            f.write("property uchar green\n")
            f.write("property uchar blue\n")
        f.write("end_header\n")
        
        # 写入点云数据
        print("正在写入点云数据...")
        write_interval = max(1, num_points // 100)  # 每1%显示一次进度
        
        for i in range(num_points):
            if add_color and colors is not None:
                f.write(f"{points[i, 0]:.6f} {points[i, 1]:.6f} {points[i, 2]:.6f} "
                       f"{colors[i, 0]} {colors[i, 1]} {colors[i, 2]}\n")
            else:
                f.write(f"{points[i, 0]:.6f} {points[i, 1]:.6f} {points[i, 2]:.6f}\n")
            
            if (i + 1) % write_interval == 0:
                progress = (i + 1) / num_points * 100
                print(f"进度: {progress:.1f}% ({i+1:,}/{num_points:,})", end='\r')
        
        print()  # 换行
    
    elapsed_time = time.time() - start_time
    file_size = os.path.getsize(output_file) / (1024 * 1024)  # MB
    
    print(f"\n生成完成！")
    print(f"文件: {output_file}")
    print(f"点数: {num_points:,}")
    print(f"文件大小: {file_size:.2f} MB")
    print(f"耗时: {elapsed_time:.2f} 秒")
    print(f"速度: {num_points/elapsed_time/1000:.1f} K点/秒")
    
    return output_file

def generate_multiple_distributions():
    """生成多种分布的点云文件用于测试"""
    distributions = {
        "random": "随机分布",
        "sphere": "球体分布",
        "cube": "立方体分布",
        "plane": "平面分布"
    }
    
    num_points = 1000000  # 100万点
    
    for dist_type, desc in distributions.items():
        print(f"\n{'='*60}")
        print(f"生成 {desc} 点云 ({dist_type})")
        print(f"{'='*60}")
        filename = f"million_points_{dist_type}.ply"
        generate_point_cloud_ply(num_points, filename, dist_type, add_color=True)
        print()

if __name__ == "__main__":
    import os
    import sys
    
    # 检查参数
    if len(sys.argv) > 1:
        if sys.argv[1] == "--all":
            # 生成所有类型的点云
            generate_multiple_distributions()
        elif sys.argv[1] == "--help" or sys.argv[1] == "-h":
            print("用法:")
            print("  python generate_million_points.py              # 生成随机分布的100万点")
            print("  python generate_million_points.py --all         # 生成所有类型的点云")
            print("  python generate_million_points.py <num>         # 生成指定数量的点")
            print("  python generate_million_points.py <num> <type>   # 生成指定数量和类型")
            print("\n分布类型: random, sphere, cube, plane")
        else:
            try:
                num_points = int(sys.argv[1])
                dist_type = sys.argv[2] if len(sys.argv) > 2 else "random"
                filename = f"million_points_{num_points//1000}K_{dist_type}.ply"
                generate_point_cloud_ply(num_points, filename, dist_type, add_color=True)
            except ValueError:
                print(f"错误: 无效的点数参数: {sys.argv[1]}")
                sys.exit(1)
            except Exception as e:
                print(f"错误: {e}")
                sys.exit(1)
    else:
        # 默认生成100万点随机分布
        generate_point_cloud_ply(1000000, "million_points.ply", "random", add_color=True)

