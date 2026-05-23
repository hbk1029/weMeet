#include "audio_write.h"

Audio_Write::Audio_Write(QObject *parent) : QObject(parent)
{
#ifdef USE_OPUS
    format.setSampleRate(48000);
#else
    format.setSampleRate(8000);
#endif
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

#ifdef USE_OPUS
    m_pOpusDecoder = new OpusDecoder(this);
    connect(m_pOpusDecoder, &OpusDecoder::sig_decodedData, this,
            [this](QByteArray pcm) {
        myBuffer_out->write(pcm.constData(), pcm.size());
    });
#else
    speex_bits_init(&bits_dec);
    Dec_State = speex_decoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));
#endif
}

Audio_Write::~Audio_Write()
{
    delete audio_out;
}

void Audio_Write::slot_net_rx(const char* data, int len)
{
#ifdef USE_OPUS
    if (m_pOpusDecoder && data && len > 0) {
        m_pOpusDecoder->slot_decode((const unsigned char*)data, len);
    }
#else
    char bytes[800] = {0};
    int i = 0;
    float output_frame1[320] = {0};
    char buf[800] = {0};
    memcpy(bytes, data, len / 2);
    speex_bits_read_from(&bits_dec, bytes, len / 2);
    int error = speex_decode(Dec_State,&bits_dec,output_frame1);
    short num = 0;
    for (i = 0;i < 160;i++)
    {
        num = output_frame1[i];
        buf[2 * i] = num;
        buf[2 * i + 1] = num >> 8;
    }
    memcpy(bytes, data + len / 2, len / 2);
    speex_bits_read_from(&bits_dec, bytes, len / 2);
    error = speex_decode(Dec_State,&bits_dec,output_frame1);
    for (i = 0;i < 160;i++)
    {
        num = output_frame1[i];
        buf[2 * i + 320] = num;
        buf[2 * i + 1 + 320] = num >> 8;
    }
    myBuffer_out->write(buf,640);
    return;
#endif
}
