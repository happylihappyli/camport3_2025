#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTS语音提示脚本
用于通知编译任务完成
"""

import os
import sys

def text_to_speech_chinese(message):
    """
    使用Windows SAPI进行中文语音合成
    """
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak(message)
        return True
    except ImportError:
        print("警告: pywin32库未安装，无法播放语音提示")
        return False
    except Exception as e:
        print(f"语音播放失败: {e}")
        return False

def main():
    message = "任务运行完毕，过来看看！"
    print(f"播放语音提示: {message}")
    
    # 尝试语音合成
    if not text_to_speech_chinese(message):
        print("语音提示不可用，但任务已完成！")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())