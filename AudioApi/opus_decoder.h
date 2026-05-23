#ifndef OPUS_DECODER_H
#define OPUS_DECODER_H

#include <QObject>

extern "C" {
#include <opus/opus.h>
}

class OpusDecoder : public QObject
{
    Q_OBJECT
public:
    explicit OpusDecoder(QObject *parent = nullptr);
    ~OpusDecoder();

signals:
    void sig_decodedData(QByteArray pcm);

public slots:
    void slot_decode(const unsigned char* data, int len);

private:
    ::OpusDecoder* m_decoder;
};

#endif // OPUS_DECODER_H
