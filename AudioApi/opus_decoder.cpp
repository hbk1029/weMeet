#include "opus_decoder.h"
#include "tracelog.h"
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
    static int callCount = 0; callCount++;
    if (!m_decoder) {
        if (callCount <= 3) TRACE("OPUS_DEC_ERR no decoder");
        return;
    }
    if (!data || len <= 0) {
        if (callCount <= 3) TRACE("OPUS_DEC_ERR bad data=%p len=%d", data, len);
        return;
    }

    // 堆分配避免栈缓冲区通过跨线程信号悬空
    opus_int16 decodedData[4096];
    // 注意：fec 固定为 0。fec=1 要求包格式含 FEC 标记，普通 Opus 包会导致 BAD_ARG(-1)
    int frameSizeDecoded = opus_decode(m_decoder,
                                       data,
                                       len,
                                       decodedData,
                                       sizeof(decodedData) / sizeof(decodedData[0]),
                                       0);
    if (frameSizeDecoded < 0) {
        static int decErrCount = 0; decErrCount++;
        if (decErrCount <= 3)
            TRACE("OPUS_DEC_ERR #%d err=%d len=%d", decErrCount, frameSizeDecoded, len);
        return;
    }

    int pcmLen = frameSizeDecoded * sizeof(opus_int16);
    static int decCount = 0; decCount++;
    if (decCount <= 5 || decCount % 100 == 0)
        TRACE("OPUS_DEC #%d opus=%dB -> pcm=%dB thread=%lu", decCount, len, pcmLen, GetCurrentThreadId());
    // 拷贝到 QByteArray 确保跨线程信号安全（深拷贝）
    QByteArray pcm(reinterpret_cast<const char*>(decodedData), pcmLen);
    if (decCount <= 5)
        TRACE("OPUS_DEC_EMIT #%d pcmSize=%d", decCount, pcm.size());
    Q_EMIT sig_decodedData(pcm);
}
