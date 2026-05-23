#include "opus_encoder.h"
#include "tracelog.h"
#include <QDebug>

OpusEncoder::OpusEncoder(QObject *parent) : QObject(parent),
    m_pEncoder(nullptr)
{
    int err;
    m_pEncoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK) {
        qDebug() << "Opus encoder create error:" << opus_strerror(err);
        return;
    }
    // 启用带内 FEC，配合解码端 fec=1 实现丢包隐藏
    opus_encoder_ctl(m_pEncoder, OPUS_SET_INBAND_FEC(1));
    // 告知编码器预期 10% 丢包率，优化码率分配和 FEC 数据量
    opus_encoder_ctl(m_pEncoder, OPUS_SET_PACKET_LOSS_PERC(10));
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
        static int skipCount = 0;
        if (++skipCount <= 3)
            qDebug() << "[OPUS_ENC] skip: enc=" << (m_pEncoder!=nullptr) << " data=" << (pcmData!=nullptr) << " len=" << len;
        return;
    }

    unsigned char opusData[4096];
    int nbBytes = opus_encode(m_pEncoder,
                               (const opus_int16*)pcmData,
                               960,
                               opusData,
                               sizeof(opusData));
    if (nbBytes < 0) {
        qDebug() << "[OPUS_ENC] encode error:" << opus_strerror(nbBytes);
        return;
    }

    static int encCount = 0; encCount++;
    if (encCount <= 3 || encCount % 100 == 0)
        TRACE("OPUS_ENC #%d pcm=%dB -> opus=%dB thread=%lu", encCount, len, nbBytes, GetCurrentThreadId());

    Q_EMIT sig_encodedData((const char*)opusData, nbBytes);
}
