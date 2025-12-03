import numpy as np
import glob

def detailed_analyze_ply(ply_file):
    print(f'详细分析: {ply_file}')
    print('='*60)
    
    try:
        with open(ply_file, 'r') as f:
            lines = f.readlines()
        
        # 找到顶点数量
        vertex_count = 0
        header_end = 0
        for i, line in enumerate(lines):
            if line.startswith('element vertex'):
                vertex_count = int(line.split()[2])
            elif line.strip() == 'end_header':
                header_end = i + 1
                break
        
        print(f'总顶点数量: {vertex_count:,}')
        
        # 分析所有点（分批处理避免内存问题）
        batch_size = 100000
        all_distances = []
        far_points = []
        
        print('正在分析所有点...')
        
        for batch_start in range(header_end, len(lines), batch_size):
            batch_lines = lines[batch_start:batch_start + batch_size]
            
            for line_idx, line in enumerate(batch_lines):
                parts = line.strip().split()
                if len(parts) >= 3:
                    try:
                        x, y, z = float(parts[0]), float(parts[1]), float(parts[2])
                        dist = np.sqrt(x*x + y*y + z*z)
                        all_distances.append(dist)
                        
                        # 记录远距离点（距离 > 10）
                        if dist > 10:
                            far_points.append({
                                'idx': batch_start - header_end + line_idx,
                                'x': x, 'y': y, 'z': z, 'dist': dist
                            })
                        
                    except:
                        continue
            
            if (batch_start - header_end) % 1000000 == 0 and batch_start > header_end:
                print(f'已处理: {(batch_start - header_end):,} 点')
        
        all_distances = np.array(all_distances)
        
        print(f'有效点数: {len(all_distances):,}')
        print(f'距离范围: [{all_distances.min():.3f}, {all_distances.max():.3f}]')
        print(f'平均距离: {all_distances.mean():.3f}')
        print(f'中位数距离: {np.median(all_distances):.3f}')
        print(f'标准差: {all_distances.std():.3f}')
        
        # 更详细的分布统计
        percentiles = [1, 5, 10, 25, 50, 75, 90, 95, 99]
        print('距离百分位数:')
        for p in percentiles:
            val = np.percentile(all_distances, p)
            print(f'  {p}%: {val:.3f}')
        
        # 远距离点分析
        print(f'远距离点数量 (>10): {len(far_points):,} ({len(far_points)/len(all_distances)*100:.3f}%)')
        
        if len(far_points) > 0:
            # 按距离排序，取最远的20个点
            far_points.sort(key=lambda p: p['dist'], reverse=True)
            
            print('最远的20个点:')
            for i, pt in enumerate(far_points[:20]):
                print(f'  {i+1:2d}: ({pt["x"]:8.2f}, {pt["y"]:8.2f}, {pt["z"]:8.2f}) - 距离: {pt["dist"]:8.3f}')
            
            # 分析远距离点的分布
            if len(far_points) > 100:
                far_dists = [p['dist'] for p in far_points]
                print(f'远距离点距离统计:')
                print(f'  范围: [{min(far_dists):.3f}, {max(far_dists):.3f}]')
                print(f'  平均: {np.mean(far_dists):.3f}')
                print(f'  中位数: {np.median(far_dists):.3f}')
        
        # 坐标范围分析
        print('坐标范围分析:')
        print('正在重新读取以分析坐标范围...')
        
        coords = {'x': [], 'y': [], 'z': []}
        for batch_start in range(header_end, len(lines), batch_size):
            batch_lines = lines[batch_start:batch_start + batch_size]
            
            for line in batch_lines:
                parts = line.strip().split()
                if len(parts) >= 3:
                    try:
                        coords['x'].append(float(parts[0]))
                        coords['y'].append(float(parts[1]))
                        coords['z'].append(float(parts[2]))
                    except:
                        continue
        
        for axis in 'xyz':
            arr = np.array(coords[axis])
            print(f'  {axis.upper()}轴: [{arr.min():8.3f}, {arr.max():8.3f}] - 范围: {arr.max()-arr.min():8.3f}')
        
    except Exception as e:
        print(f'分析出错: {e}')
        import traceback
        traceback.print_exc()

# 分析最新的点云文件
latest_files = sorted(glob.glob('2025.12.1*.ply'))
if latest_files:
    detailed_analyze_ply(latest_files[-1])  # 分析最新的文件
else:
    print('未找到最新的点云文件')