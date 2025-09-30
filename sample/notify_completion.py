import winsound
import time

# 播放提示音通知用户程序已成功修改并编译完成
def play_notification_sound():
    # 播放第一声提示音（1000Hz，300毫秒）
    winsound.Beep(1000, 300)
    time.sleep(0.2)  # 短暂停顿
    # 播放第二声提示音（1500Hz，300毫秒）
    winsound.Beep(1500, 300)

if __name__ == "__main__":
    print("\n\n========= 通知 ========\n")
    print("PointCloud_NoCV.cpp 文件已成功修改并编译完成！")
    print("\n已实现的功能：")
    print("1. 修复了重复的 TYStopCapture 调用")
    print("2. 添加了 RGBD 配准功能，使用 TYMapRGBImageToDepthCoordinate API")
    print("3. 支持获取相机标定信息并用于配准")
    print("4. 添加了 registeredRgbData 缓冲区来存储配准后的彩色数据")
    print("5. 修改了 savePointsToPly 函数调用，优先使用配准后的彩色数据")
    print("\n现在可以运行 bin\PointCloud_NoCV.exe 来测试彩色点云保存功能！")
    print("\n======================\n")
    
    # 播放提示音
    play_notification_sound()
    
    # 等待用户查看信息
    time.sleep(2)