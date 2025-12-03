#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import chardet

def fix_file_encoding(file_path):
    """修复文件编码为UTF-8 BOM"""
    try:
        # 检测当前编码
        with open(file_path, 'rb') as f:
            raw_data = f.read()
            result = chardet.detect(raw_data)
            encoding = result['encoding']
            print(f"文件 {file_path} 当前编码: {encoding}")
        
        # 读取文件内容
        with open(file_path, 'r', encoding=encoding) as f:
            content = f.read()
        
        # 写入UTF-8 BOM格式
        with open(file_path, 'w', encoding='utf-8-sig') as f:
            f.write(content)
            
        print(f"文件 {file_path} 已转换为 UTF-8-BOM 编码")
        
    except Exception as e:
        print(f"处理文件 {file_path} 时出错: {e}")

def main():
    # 需要修复编码的文件列表
    files_to_fix = [
        'E:\\github\\camport3_2025\\sample\\common\\funny_Mat.hpp',
        'E:\\github\\camport3_2025\\sample\\common\\common.hpp'
    ]
    
    for file_path in files_to_fix:
        if os.path.exists(file_path):
            fix_file_encoding(file_path)
        else:
            print(f"文件不存在: {file_path}")

if __name__ == '__main__':
    main()