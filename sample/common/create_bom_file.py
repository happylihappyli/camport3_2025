import os

# 读取原始文件内容
with open('funny_Mat.hpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 创建带有BOM的新文件
with open('funny_Mat_bom.hpp', 'w', encoding='utf-8-sig') as f:
    f.write(content)

print('文件已创建: funny_Mat_bom.hpp')