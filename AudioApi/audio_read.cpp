#include "audio_read.h"

Audio_Read::Audio_Read(QObject *parent) : QObject(parent),
    m_pTimer(nullptr), m_pAudioIn(nullptr), m_pBufferIn(nullptr)
{
    //声卡采样格式
    m_format.setSampleRate(8000);
    m_format.setChannelCount(1);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(m_format)) {
        m_format = info.nearestFormat(m_format);
    }
    m_recordState = audio_state::_Stop;

    //Speex 编码器初始化
    speex_bits_init(&m_bitsEnc);
    m_pEncState = speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));
    int tmp = SPEEX_QUALITY;
    speex_encoder_ctl(m_pEncState, SPEEX_SET_QUALITY, &tmp);
}

Audio_Read::~Audio_Read()
{
    if (m_pAudioIn) {
        m_pAudioIn->stop();
        delete m_pAudioIn;
    }
    if (m_pTimer) {
        m_pTimer->stop();
        delete m_pTimer;
    }
}

void Audio_Read::slot_startRecord()
{
    if (m_recordState != audio_state::_Record) {
        m_pAudioIn = new QAudioInput(m_format, this);
        m_pBufferIn = m_pAudioIn->start();
        if (!m_pBufferIn) {
            delete m_pAudioIn;
            m_pAudioIn = nullptr;
            return;
        }
        m_pTimer = new QTimer(this);
        connect(m_pTimer, &QTimer::timeout, this, &Audio_Read::slot_readMore);
        m_pTimer->start(20);
    }
    m_recordState = audio_state::_Record;
}

void Audio_Read::slot_pauseRecord()
{
    if (m_recordState == audio_state::_Record) {
        if (m_pAudioIn) {
            m_pAudioIn->stop();
            delete m_pAudioIn;
            m_pAudioIn = nullptr;
        }
        if (m_pTimer) {
            m_pTimer->stop();
            delete m_pTimer;
            m_pTimer = nullptr;
        }
    }
    m_recordState = audio_state::_Pause;
}

void Audio_Read::slot_readMore()
{
#ifndef USE_SPEEX

    if (!m_pAudioIn || !m_pBufferIn)
        return;

    QByteArray m_buffer(2048,0);
    qint64 len = m_pAudioIn->bytesReady();
    if (len < 640)
    {
        return;
    }
    qint64 l = m_pBufferIn->read(m_buffer.data(), 640);
    //qDebug() << "l sizes:"<< l ;
    QByteArray frame;
    frame.append(m_buffer.data(),640);

    Q_EMIT sig_audioFrame(frame.data(), frame.size());
#else
    if (!m_pAudioIn || !m_pBufferIn)
        return;

    QByteArray m_buffer(2048,0);
    qint64 len = m_pAudioIn->bytesReady();
    if (len < 640)
    {
        return;
    }
    qint64 l = m_pBufferIn->read(m_buffer.data(), 640);
    //qDebug() << "l sizes:"<< l ;

    QByteArray frame;
    char bytes[800] = {0};
    int i = 0;
    float input_frame1[320];
    char* pInData = (char*)m_buffer.data();
    char buf[800] = {0};
    memcpy(buf, pInData, 640);

    int nbytes = 0;
    {
        short num = 0;
        for (i = 0;i < 160;i++)
        {
            num = (uint8_t)buf[2 * i] | (((uint8_t)buf[2 * i + 1]) << 8);
            input_frame1[i] = num;
        }
        speex_bits_reset(&m_bitsEnc);
        speex_encode(m_pEncState, input_frame1, &m_bitsEnc);
        nbytes = speex_bits_write(&m_bitsEnc, bytes, 800);
        frame.append(bytes, nbytes);
        for (i = 0;i < 160;i++)
        {
            num = (uint8_t)buf[2 * i + 320] | (((uint8_t)buf[2 * i + 1 + 320]) << 8);
            input_frame1[i] = num;
        }
        speex_bits_reset(&m_bitsEnc);
        speex_encode(m_pEncState, input_frame1, &m_bitsEnc);
        nbytes = speex_bits_write(&m_bitsEnc, bytes, 800);
        frame.append(bytes, nbytes);
        //qDebug() << "nbytes = " << frame.size();
    }

    Q_EMIT sig_audioFrame(frame.data(), frame.size());
#endif
}
