#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复PointCloud项目main.cpp文件的编码问题
将文件转换为UTF-8+BOM编码格式
"""

import os
import codecs

def fix_file_encoding(file_path):
    """修复文件的编码问题"""
    try:
        # 读取原文件，假设是UTF-8编码
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        print(f"成功读取文件: {file_path}")
        print(f"文件大小: {len(content)} 字符")
        
        # 将文件保存为UTF-8+BOM格式
        with codecs.open(file_path, 'w', encoding='utf-8-sig') as f:
            f.write(content)
        
        print(f"成功保存为UTF-8+BOM格式: {file_path}")
        return True
        
    except UnicodeDecodeError as e:
        print(f"编码错误: {e}")
        # 如果UTF-8解码失败，尝试其他编码
        try:
            with open(file_path, 'r', encoding='gbk') as f:
                content = f.read()
            
            print(f"使用GBK编码读取文件成功: {file_path}")
            
            # 将文件保存为UTF-8+BOM格式
            with codecs.open(file_path, 'w', encoding='utf-8-sig') as f:
                f.write(content)
            
            print(f"成功转换为UTF-8+BOM格式: {file_path}")
            return True
            
        except Exception as e2:
            print(f"GBK编码也失败: {e2}")
            return False
            
    except Exception as e:
        print(f"处理文件时出错: {e}")
        return False

def main():
    """主函数"""
    # PointCloud项目的main.cpp文件路径
    main_cpp_path = r"E:\github\camport3_2025\sample\sample_v2\sample\PointCloud\main.cpp"
    
    print("=== PointCloud项目编码修复工具 ===")
    print(f"目标文件: {main_cpp_path}")
    
    # 检查文件是否存在
    if not os.path.exists(main_cpp_path):
        print(f"错误: 文件不存在 {main_cpp_path}")
        return False
    
    # 修复文件编码
    success = fix_file_encoding(main_cpp_path)
    
    if success:
        print("✅ 编码修复成功!")
        print("文件已转换为UTF-8+BOM编码格式")
    else:
        print("❌ 编码修复失败!")
    
    return success

if __name__ == "__main__":
    main()