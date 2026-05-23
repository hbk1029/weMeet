#ifndef SDL_AUDIO_READ_H
#define SDL_AUDIO_READ_H

#include <QObject>
#include <QByteArray>
#include "audio_common.h"

extern "C" {
#include <SDL.h>
}

class VadWrapper;

#ifdef USE_SDL2

class SDLAudioRead : public QObject
{
    Q_OBJECT
public:
    explicit SDLAudioRead(QObject *parent = nullptr);
    ~SDLAudioRead();

signals:
    void SIG_sendAudioFrame(QByteArray data);

public slots:
    void slot_openAudio();
    void slot_closeAudio();

private:
    static void audioCallback(void *userdata, Uint8 *stream, int len);

    SDL_AudioDeviceID m_dev;
    bool m_isOpen;
#ifdef USE_VAD
    VadWrapper* m_pVad;
#endif
};

#endif // USE_SDL2
#endif // SDL_AUDIO_READ_H
