import os
import re

# 定义要替换的映射
replacements = [
    (r'FastCamera::stream_depth', r'FastCamera::stream_idx::depth'),
    (r'FastCamera::stream_color', r'FastCamera::stream_idx::color'),
    (r'FastCamera::stream_ir_left', r'FastCamera::stream_idx::ir_left'),
    (r'FastCamera::stream_ir_right', r'FastCamera::stream_idx::ir_right'),
    (r'FastCamera::stream_ir', r'FastCamera::stream_idx::ir'),
    (r'stream_depth', r'FastCamera::stream_idx::depth'),
    (r'stream_color', r'FastCamera::stream_idx::color'),
    (r'stream_ir_left', r'FastCamera::stream_idx::ir_left'),
    (r'stream_ir_right', r'FastCamera::stream_idx::ir_right'),
    (r'stream_ir', r'FastCamera::stream_idx::ir'),
]

# 要处理的文件列表
files_to_update = [
    r"E:\github\camport3_2025\sample\sample_v2\sample\ResolutionSetting\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\TofDepthStream\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\PointCloud\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\OpenWithInterface\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\IREnhance\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\ExposureTimeSetting\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\DepthStream\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\OpenWithIP\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\StreamAsync\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\OfflineReconnection\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\MultiDeviceOfflineReconnection\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\Registration\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\NetStatistic\main.cpp",
    r"E:\github\camport3_2025\sample\sample_v2\sample\SoftTrigger\main.cpp",
]

# 处理每个文件
for file_path in files_to_update:
    if os.path.exists(file_path):
        with open(file_path, 'r', encoding='utf-8') as file:
            content = file.read()
        
        # 应用所有替换
        for pattern, replacement in replacements:
            content = re.sub(pattern, replacement, content)
        
        # 写回文件
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(content)
        
        print(f"已更新: {file_path}")
    else:
        print(f"文件不存在: {file_path}")

print("所有文件更新完成！")