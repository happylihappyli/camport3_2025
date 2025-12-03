import pyttsx3

def play_completion_sound():
    """播放任务完成语音提示"""
    try:
        engine = pyttsx3.init()
        engine.say("任务运行完毕，过来看看！")
        engine.runAndWait()
    except Exception as e:
        print(f"播放语音提示失败: {e}")

if __name__ == "__main__":
    play_completion_sound()