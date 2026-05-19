#include "opus_decoder.h"
#include <QDebug>

OpusDecoder::OpusDecoder(QObject *parent) : QObject(parent), m_decoder(nullptr)
{
    int err;
    m_decoder = opus_decoder_create(48000, 1, &err);
    if (err != OPUS_OK) {
        qDebug() << "opus_decoder_create failed:" << opus_strerror(err);
    }
}

OpusDecoder::~OpusDecoder()
{
    if (m_decoder) {
        opus_decoder_destroy(m_decoder);
        m_decoder = nullptr;
    }
}

void OpusDecoder::slot_decode(const unsigned char* data, int len)
{
    if (!m_decoder || !data || len <= 0)
        return;

    opus_int16 decodedData[4096];
    int frameSizeDecoded = opus_decode(m_decoder,
                                       data,
                                       len,
                                       decodedData,
                                       sizeof(decodedData) / sizeof(decodedData[0]),
                                       0);
    if (frameSizeDecoded < 0) {
        qDebug() << "opus_decode error:" << opus_strerror(frameSizeDecoded);
        return;
    }

    int pcmLen = frameSizeDecoded * sizeof(opus_int16);
    Q_EMIT sig_decodedData(reinterpret_cast<const char*>(decodedData), pcmLen);
}
