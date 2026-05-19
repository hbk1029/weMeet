#ifndef AUDIO_WRITE_H
#define AUDIO_WRITE_H

#include <QObject>
#include "audio_common.h"

#ifdef USE_OPUS
#include "opus_decoder.h"
#endif

class Audio_Write : public QObject
{
    Q_OBJECT
public:
    explicit Audio_Write(QObject *parent = nullptr);
    ~Audio_Write();
signals:

public slots:
    void slot_net_rx(const char* data, int len);
private:
    QAudioFormat format;

    QAudioOutput*   audio_out;
    QIODevice*      myBuffer_out;

#ifdef USE_OPUS
    OpusDecoder*    m_pOpusDecoder;
#else
    SpeexBits bits_dec;
    void *Dec_State;
#endif
};

#endif // AUDIO_WRITE_H
