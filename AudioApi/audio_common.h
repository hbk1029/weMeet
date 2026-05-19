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

#ifdef USE_OPUS
extern "C" {
#include <opus/opus.h>
}
#else
#include <speex/include/speex.h>
#define SPEEX_QUALITY (8)
#endif

#endif // AUDIO_COMMON_H
