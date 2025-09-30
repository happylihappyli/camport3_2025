# -*- coding: utf-8 -*-
import subprocess
import time
import sys
import os

# 测试building terminated检测逻辑
# 这个脚本模拟scons构建过程中输出"building terminated"的情况

def test_termination_detection():
    # 创建一个模拟构建过程的批处理文件，使用ANSI编码以避免Windows命令行中的编码问题
    batch_content = '''
@echo off
chcp 65001 > nul

rem 模拟构建输出
for /l %%i in (1,1,5) do (
    echo Compiling file%%i...
    timeout /t 1 > nul
)

echo Error occurred during compilation

echo scons: building terminated because of errors

rem 故意不立即退出，模拟进程卡住的情况
rem timeout /t 10 > nul
'''
    
    batch_file = 'mock_build.bat'
    # 在Windows上，批处理文件通常使用ANSI编码
    with open(batch_file, 'w', encoding='cp936') as f:
        f.write(batch_content)
    
    print("启动模拟构建进程...")
    process = subprocess.Popen(
        [batch_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding='utf-8',
        errors='replace'  # 替换无法解码的字符
    )
    
    output_lines = []
    # 添加标志变量和计时器，与build_with_error_check.py中的逻辑保持一致
    building_terminated_detected = False
    termination_time = 0
    max_wait_time_after_termination = 3  # 检测到terminated后最多等待3秒
    
    print("进入循环读取输出...")
    while True:
        # 首先检查进程是否已经结束
        return_code = process.poll()
        if return_code is not None:
            print(f"进程已结束，退出码: {return_code}")
            # 读取剩余所有输出
            remaining_output = process.stdout.read()
            if remaining_output:
                lines = remaining_output.split('\n')
                for line in lines:
                    if line.strip():
                        print(f"读取剩余输出: {line}")
                        output_lines.append(line)
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
                        print(f"读取剩余输出: {line}")
                        output_lines.append(line)
            break
        
        try:
            # 尝试读取一行输出
            line = process.stdout.readline()
            if line:
                line_str = line.strip()
                print(f"读取输出行: {line_str}")
                output_lines.append(line_str)
                
                # 检测是否包含"building terminated"字符串
                if 'building terminated' in line_str.lower():
                    print("检测到构建终止信息")
                    building_terminated_detected = True
                    termination_time = time.time()
            else:
                # 短暂休眠避免CPU占用过高
                time.sleep(0.1)
        except Exception as e:
            print(f"读取输出时出错: {e}")
            time.sleep(0.1)
    
    print(f"总共读取到{len(output_lines)}行输出")
    print("测试完成。")
    
    # 清理临时文件
    try:
        os.remove(batch_file)
    except:
        pass

if __name__ == "__main__":
    test_termination_detection()