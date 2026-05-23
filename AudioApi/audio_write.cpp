#include "audio_write.h"

Audio_Write::Audio_Write(QObject *parent) : QObject(parent)
{
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if (!info.isFormatSupported(format)) {
        format = info.nearestFormat(format);
    }

    audio_out    = new QAudioOutput(format, this);
    myBuffer_out = audio_out->start();

    m_pOpusDecoder = new OpusDecoder(this);
    connect(m_pOpusDecoder, &OpusDecoder::sig_decodedData, this,
            [this](QByteArray pcm) {
        myBuffer_out->write(pcm.constData(), pcm.size());
    });
}

Audio_Write::~Audio_Write()
{
    delete audio_out;
}

void Audio_Write::slot_net_rx(const char* data, int len)
{
    if (m_pOpusDecoder && data && len > 0) {
        m_pOpusDecoder->slot_decode((const unsigned char*)data, len);
    }
}
