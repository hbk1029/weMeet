#include "audio_read.h"

Audio_Read::Audio_Read(QObject *parent) : QObject(parent),
    m_pTimer(nullptr), m_pAudioIn(nullptr), m_pBufferIn(nullptr)
{
    m_format.setSampleRate(48000);
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

    m_pOpusEncoder = new OpusEncoder(this);
    connect(m_pOpusEncoder, &OpusEncoder::sig_encodedData,
            this, &Audio_Read::sig_audioFrame);
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
    if (!m_pAudioIn || !m_pBufferIn)
        return;

    QByteArray m_buffer(4096, 0);
    qint64 len = m_pAudioIn->bytesReady();
    if (len < 1920) return;

    qint64 l = m_pBufferIn->read(m_buffer.data(), 1920);
    Q_UNUSED(l);
    m_pOpusEncoder->slot_encode(m_buffer.data(), 1920);
}
