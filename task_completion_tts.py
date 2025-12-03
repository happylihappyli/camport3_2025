#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTS语音提示脚本
用于告知用户任务完成情况
"""

import pyttsx3
import time

def speak_completion_message():
    """生成任务完成的语音提示"""
    try:
        # 初始化TTS引擎
        engine = pyttsx3.init()
        
        # 设置中文语音
        voices = engine.getProperty('voices')
        for voice in voices:
            if 'chinese' in voice.name.lower() or 'chinese' in voice.id.lower():
                engine.setProperty('voice', voice.id)
                break
        
        # 设置语速
        engine.setProperty('rate', 180)
        
        # 生成完成信息
        completion_message = "任务运行完毕，过来看看！已成功修复所有TY_PIXEL_FORMAT_LIST语法错误，包括Frame.cpp和IREnhance.hpp文件中的相关错误。所有枚举值现在都使用正确的直接引用语法。"
        
        # 语音播放
        engine.say(completion_message)
        engine.runAndWait()
        
        print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] 语音提示已播放: {completion_message}")
        
    except Exception as e:
        print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] TTS语音提示生成失败: {e}")
        print("替代文本提示: 任务运行完毕，过来看看！已成功修复所有TY_PIXEL_FORMAT_LIST语法错误。")

if __name__ == "__main__":
    speak_completion_message()