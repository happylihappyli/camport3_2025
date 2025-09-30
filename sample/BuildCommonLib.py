import os
from os.path import join, abspath, exists
from SCons.Script import *

# 设置构建环境
env = Environment()

# 设置基本路径
LIB_ROOT_PATH = abspath('../lib/win/hostapp/')
INCLUDE_PATH = abspath('../include/')
COMMON_DIR = abspath('./common/')

# 添加头文件路径
env.Append(CPPPATH=[INCLUDE_PATH, COMMON_DIR])

# 定义WIN32宏
env.Append(CPPDEFINES=['WIN32'])

# 添加编译选项
env.Append(CXXFLAGS=['/EHsc'])

# 架构选择
lib_arch = 'x64'  # 使用64位架构
lib_path = join(LIB_ROOT_PATH, lib_arch)
env.Append(LIBPATH=[lib_path])

# 定义common源文件
common_sources = [
    join(COMMON_DIR, 'MatViewer.cpp'),
    join(COMMON_DIR, 'TYThread.cpp'),
    join(COMMON_DIR, 'crc32.cpp'),
    join(COMMON_DIR, 'json11.cpp'),
    join(COMMON_DIR, 'ParametersParse.cpp'),
    join(COMMON_DIR, 'huffman.cpp'),
    join(COMMON_DIR, 'ImageSpeckleFilter.cpp'),
    join(COMMON_DIR, 'DepthInpainter.cpp'),
    join(COMMON_DIR, 'funny_resize.cpp'),
]

# 确保所有源文件存在
for src in common_sources:
    if not os.path.exists(src):
        print(f"Warning: Source file not found: {src}")

# 构建common_lib库
common_lib = env.Library('common_lib', common_sources)
print("Successfully created common_lib library builder")

# 构建一个简单的测试程序来验证common_lib是否可用
test_source = """
#include <iostream>
#include "funny_Mat.hpp"

int main() {
    std::cout << "Testing common_lib..." << std::endl;
    funny_Mat mat(640, 480, CV_8UC3, NULL);
    std::cout << "Created funny_Mat object" << std::endl;
    return 0;
}
"""

# 写入测试源文件
test_file = join(COMMON_DIR, 'test_common_lib.cpp')
with open(test_file, 'w') as f:
    f.write(test_source)

# 构建测试程序
test_program = env.Program('test_common_lib', [test_file, common_lib])
print("Successfully created test_common_lib program builder")