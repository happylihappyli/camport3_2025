# -*- coding: utf-8 -*-
import subprocess
import time
import sys

# 测试循环退出逻辑
# 这个脚本模拟一个命令执行后终止的情况，验证我们的循环逻辑能否正确退出

def test_loop_exit():
    # 创建一个会很快退出的进程
    print("启动测试进程...")
    process = subprocess.Popen(
        ['cmd', '/c', 'echo 测试输出 && echo 这是第二行输出 && exit 1'],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    
    output_lines = []
    loop_count = 0
    max_loops = 100  # 防止无限循环
    
    # 测试我们修复后的循环逻辑
    print("进入循环读取输出...")
    while loop_count < max_loops:
        loop_count += 1
        
        # 检查进程是否已经结束
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
        
        try:
            # 尝试读取一行输出
            line = process.stdout.readline()
            if line:
                print(f"读取输出行: {line.strip()}")
                output_lines.append(line.strip())
            else:
                # 短暂休眠
                time.sleep(0.1)
        except Exception as e:
            print(f"读取输出时出错: {e}")
            time.sleep(0.1)
    
    if loop_count >= max_loops:
        print("警告: 达到最大循环次数，可能存在循环退出问题！")
    else:
        print(f"成功: 循环在{loop_count}次迭代后退出")
    
    print(f"总共读取到{len(output_lines)}行输出")
    print("测试完成。")

if __name__ == "__main__":
    test_loop_exit()