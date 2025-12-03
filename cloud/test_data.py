import os
import struct
import datetime

def create_test_depth_data():
    """创建测试深度数据文件"""
    width, height = 1280, 960
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # 创建深度数据（16位深度值）
    depth_data = []
    for y in range(height):
        for x in range(width):
            # 创建简单的测试模式：中心区域深度较小，边缘深度较大
            center_x, center_y = width // 2, height // 2
            distance = ((x - center_x)**2 + (y - center_y)**2)**0.5
            depth_value = min(5000, int(1000 + distance * 2))  # 毫米为单位
            depth_data.append(depth_value)
    
    # 保存深度数据
    depth_filename = f"depth_{timestamp}.raw"
    with open(depth_filename, 'wb') as f:
        for value in depth_data:
            f.write(struct.pack('<H', value))  # 小端16位无符号整数
    
    # 保存元信息
    meta_filename = f"depth_{timestamp}.meta"
    with open(meta_filename, 'w') as f:
        f.write(f"width={width}\n")
        f.write(f"height={height}\n")
        f.write(f"pixelFormat={0x20000000}\n")  # TY_PIXEL_FORMAT_DEPTH16
        f.write(f"size={width * height * 2}\n")  # 每个像素2字节
    
    print(f"创建测试深度数据: {depth_filename} ({width}x{height}, {width*height*2}字节)")

def create_test_color_data():
    """创建测试彩色数据文件"""
    width, height = 1280, 960
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # 创建彩色数据（RGB888格式）
    color_data = []
    for y in range(height):
        for x in range(width):
            # 创建简单的颜色渐变
            r = min(255, int(x * 255 / width))
            g = min(255, int(y * 255 / height))
            b = 128
            color_data.extend([b, g, r])  # BGR格式
    
    # 保存彩色数据
    color_filename = f"color_{timestamp}.raw"
    with open(color_filename, 'wb') as f:
        f.write(bytes(color_data))
    
    # 保存元信息
    meta_filename = f"color_{timestamp}.meta"
    with open(meta_filename, 'w') as f:
        f.write(f"width={width}\n")
        f.write(f"height={height}\n")
        f.write(f"pixelFormat={0x30000000}\n")  # TY_PIXEL_FORMAT_RGB
        f.write(f"size={width * height * 3}\n")  # 每个像素3字节
    
    print(f"创建测试彩色数据: {color_filename} ({width}x{height}, {width*height*3}字节)")

if __name__ == "__main__":
    create_test_depth_data()
    create_test_color_data()
    print("测试数据文件创建完成！")