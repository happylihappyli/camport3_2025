import winsound
import time

def play_completion_sound():
    """播放任务完成提示音"""
    try:
        # 播放两个不同频率的提示音
        winsound.Beep(440, 200)  # A4音符，200ms
        time.sleep(0.1)            # 暂停100ms
        winsound.Beep(523, 300)  # C5音符，300ms
        
        print("任务完成提示音播放完毕！")
        
    except Exception as e:
        print(f"播放提示音失败: {e}")

if __name__ == "__main__":
    play_completion_sound()