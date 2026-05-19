#include "opus_encoder.h"
#include <QDebug>

OpusEncoder::OpusEncoder(QObject *parent) : QObject(parent),
    m_pEncoder(nullptr)
{
    int err;
    m_pEncoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK) {
        qDebug() << "Opus encoder create error:" << opus_strerror(err);
    }
}

OpusEncoder::~OpusEncoder()
{
    if (m_pEncoder) {
        opus_encoder_destroy(m_pEncoder);
        m_pEncoder = nullptr;
    }
}

void OpusEncoder::slot_encode(const char* pcmData, int len)
{
    if (!m_pEncoder || !pcmData || len < 1920) {
        return;
    }

    unsigned char opusData[4096];
    int nbBytes = opus_encode(m_pEncoder,
                               (const opus_int16*)pcmData,
                               960,
                               opusData,
                               sizeof(opusData));
    if (nbBytes < 0) {
        qDebug() << "Opus encode error:" << opus_strerror(nbBytes);
        return;
    }

    Q_EMIT sig_encodedData((const char*)opusData, nbBytes);
}
