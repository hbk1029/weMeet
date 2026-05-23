#ifndef AUDIO_COMMON_H
#define AUDIO_COMMON_H

#include<QAudioInput>
#include<QAudioOutput>
#include<QAudioFormat>
#include<QAudioDeviceInfo>
#include<QMessageBox>
#include<qDebug>

// 注释此行回退到 Speex 8kHz
#define USE_OPUS

// 注释此行回退到 Qt QAudioInput/QAudioOutput
#define USE_SDL2

// 注释此行关闭 VAD 语音检测（当前已关闭：避免丢帧导致杂音）
//#define USE_VAD

#ifdef USE_OPUS
extern "C" {
#include <opus/opus.h>
}
#else
#include <speex/include/speex.h>
#define SPEEX_QUALITY (8)
#endif

#endif // AUDIO_COMMON_H
