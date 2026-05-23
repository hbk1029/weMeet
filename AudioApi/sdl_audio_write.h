#ifndef SDL_AUDIO_WRITE_H
#define SDL_AUDIO_WRITE_H

#include <QObject>
#include <QByteArray>
#include <list>
#include <mutex>
#include "audio_common.h"
#include "opus_decoder.h"

extern "C" {
#include <SDL.h>
}

#ifdef USE_SDL2

class SDLAudioWrite : public QObject
{
    Q_OBJECT
public:
    explicit SDLAudioWrite(QObject *parent = nullptr);
    ~SDLAudioWrite();

public slots:
    void slot_openAudio();
    void slot_closeAudio();
    void slot_net_rx(const char* data, int len);
    void slot_playAudioFrame(QByteArray recvBuffer);

private:
    static void audioCallback(void *userdata, Uint8 *stream, int len);

    SDL_AudioDeviceID m_dev;
    bool m_isOpen;
    bool m_isBuffered;  // 预缓冲完成标志，缓冲足够帧后才开始播放
    int m_preBufCount;  // 预缓冲帧计数
    std::list<QByteArray> m_audioQueue;
    std::mutex m_mutex;
    OpusDecoder* m_pOpusDecoder;
    static constexpr int kMaxQueueSize = 6;   // 最大缓冲 6 帧 = 120ms，超出丢弃旧帧
    static constexpr int kPreBufTarget = 1;   // 有至少 1 帧就开始播放，避免初始空白爆破音
};

#endif // USE_SDL2
#endif // SDL_AUDIO_WRITE_H
