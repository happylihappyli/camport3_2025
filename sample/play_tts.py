#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTS语音提示脚本
用于播放任务完成或重要操作的语音提示
"""

import subprocess
import platform
import sys

def play_tts(text, lang="zh-CN"):
    """播放TTS语音提示"""
    if platform.system() == "Windows":
        # Windows系统使用PowerShell的SpeechSynthesizer
        command = f'Add-Type -AssemblyName System.speech; $synth = New-Object System.Speech.Synthesis.SpeechSynthesizer; $synth.Speak("{text}")'
        try:
            subprocess.run(["powershell", "-Command", command], 
                         shell=True, check=True)
        except subprocess.CalledProcessError:
            print(f"语音提示播放失败: {text}")
    else:
        print(f"TTS提示: {text}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # 从命令行参数获取要播放的文本
        text = " ".join(sys.argv[1:])
    else:
        # 默认提示文本
        text = "任务运行完毕，过来看看！"
    
    play_tts(text)