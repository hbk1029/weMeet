#ifndef AUDIO_WRITE_H
#define AUDIO_WRITE_H

#include <QObject>
#include "audio_common.h"
#include "opus_decoder.h"

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
    OpusDecoder*    m_pOpusDecoder;
};

#endif // AUDIO_WRITE_H
