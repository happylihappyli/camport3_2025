#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import subprocess
import re
import sys
import os
import time

# 运行构建命令并捕获输出和错误
def run_build():
    start_time = time.time()
    
    # 运行scons命令
    print("开始构建Camport3示例项目...")
    process = subprocess.Popen(
        ['scons'],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding='gb2312',
        errors='replace',  # 替换无法解码的字符，确保在Windows环境下正常工作
        cwd=os.path.dirname(os.path.abspath(__file__))
    )
    
    # 实时输出构建过程
    build_output = []
    # 添加标志变量和计时器，用于实现用户建议的退出逻辑
    building_terminated_detected = False
    termination_time = 0
    max_wait_time_after_termination = 1  # 检测到terminated后最多等待1秒
    
    while True:
        # 首先检查进程是否已经结束
        return_code = process.poll()
        if return_code is not None:
            # 进程已结束，读取剩余所有输出
            remaining_output = process.stdout.read()
            if remaining_output:
                lines = remaining_output.split('\n')
                for line in lines:
                    if line.strip():
                        print(line.strip())
                        build_output.append(line + '\n')
            break
        
        # 检查是否已经过了强制退出时间
        if building_terminated_detected and (time.time() - termination_time > max_wait_time_after_termination):
            print(f"检测到构建终止后已等待超过{max_wait_time_after_termination}秒，强制退出循环")
            # 读取剩余所有输出
            remaining_output = process.stdout.read()
            if remaining_output:
                lines = remaining_output.split('\n')
                for line in lines:
                    if line.strip():
                        print(line.strip())
                        build_output.append(line + '\n')
            break
        
        try:
            # 尝试读取一行输出
            line = process.stdout.readline()
            if line:
                line_str = line.strip()
                print(line_str)
                build_output.append(line)
                
                # 检测是否包含"building terminated"字符串
                if 'building terminated' in line_str.lower():
                    print("检测到构建终止信息")
                    building_terminated_detected = True
                    termination_time = time.time()
                if 'end compile' in line_str.lower():
                    print("检测到构建终止信息")
                    building_terminated_detected = True
                    termination_time = time.time()
            else:
                # 短暂休眠避免CPU占用过高
                time.sleep(0.1)
        except Exception as e:
            print(f"读取输出时出错: {e}")
            time.sleep(0.1)
        if building_terminated_detected:
            break
    # 生成编译总结
    generate_summary(build_output, return_code, time.time() - start_time)
    
    # 返回退出码
    return return_code

# 生成编译总结
def generate_summary(build_output, return_code, elapsed_time):
    # 合并构建输出为单个字符串
    output_str = '\n'.join(build_output)
    
    # 统计编译错误和警告
    errors = re.findall(r'error|致命错误', output_str, re.IGNORECASE)
    warnings = re.findall(r'warning', output_str, re.IGNORECASE)
    
    # 检查是否有构建终止的信息
    build_terminated = 'building terminated because of errors' in output_str.lower()
    
    # 检查bin目录中的可执行文件
    bin_dir = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'bin'))
    sample_v1_exes = []
    sample_v2_exes = []
    
    if os.path.exists(bin_dir):
        executables = [f for f in os.listdir(bin_dir) if f.endswith('.exe')]
        sample_v1_exes = [f for f in executables if not f.endswith('_v2.exe')]
        sample_v2_exes = [f for f in executables if f.endswith('_v2.exe')]
    
    # 确定编译状态
    success = return_code == 0 and not build_terminated
    
    # 打印编译总结
    print("\n" + "=" * 80)
    print("                    Camport3 编译总结                    ")
    print("=" * 80)
    print(f"编译耗时: {elapsed_time:.2f} 秒")
    print(f"构建命令退出码: {return_code}")
    
    if sample_v1_exes or sample_v2_exes:
        print(f"\n成功编译的可执行文件:")
        print(f"Sample_v1 可执行文件数量: {len(sample_v1_exes)}")
        for exe in sample_v1_exes:
            print(f"  - {exe}")
            
        print(f"Sample_v2 可执行文件数量: {len(sample_v2_exes)}")
        for exe in sample_v2_exes:
            print(f"  - {exe}")
    
    print(f"\n编译统计:")
    print(f"编译错误数量: {len(errors)}")
    print(f"编译警告数量: {len(warnings)}")
    print(f"构建是否终止: {'是' if build_terminated else '否'}")
    
    print(f"\n编译状态: {'成功' if success else '失败'}")
    
    print("=" * 80)
    print("编译总结已完成。")
    print("=" * 80)

# 主函数
if __name__ == "__main__":
    exit_code = run_build()
    sys.exit(exit_code)