#include "wav_player.h"
#include <QAudioFormat>
#include <QDebug>

WavPlayer::WavPlayer(QObject *parent)
    : QObject(parent)
    , m_isPlaying(false)
    , m_file(nullptr)
    , m_audioOutput(nullptr)
    , m_buffer(nullptr)
{
}

WavPlayer::~WavPlayer()
{
    stop();
}

// 简单的 WAV 播放器：读取整个文件，用 QAudioOutput 播放
// 只支持标准 PCM WAV（44 字节头）
void WavPlayer::play(const QString& filePath)
{
    stop();

    m_file = new QFile(filePath, this);
    if (!m_file->open(QIODevice::ReadOnly)) {
        qWarning() << "WavPlayer: cannot open" << filePath;
        delete m_file;
        m_file = nullptr;
        return;
    }

    // 解析 WAV 头
    QByteArray header = m_file->read(44);
    if (header.size() < 44) {
        qWarning() << "WavPlayer: invalid WAV header";
        delete m_file;
        m_file = nullptr;
        return;
    }

    int sampleRate = *reinterpret_cast<const int*>(header.constData() + 24);
    short channels = *reinterpret_cast<const short*>(header.constData() + 22);
    short bitsPerSample = *reinterpret_cast<const short*>(header.constData() + 34);
    int dataSize = *reinterpret_cast<const int*>(header.constData() + 40);

    // 读取 PCM 数据
    QByteArray pcmData = m_file->read(dataSize);
    m_file->close();
    delete m_file;
    m_file = nullptr;

    if (pcmData.isEmpty()) {
        qWarning() << "WavPlayer: no PCM data";
        return;
    }

    // 配置音频格式
    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleSize(bitsPerSample);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if (!info.isFormatSupported(format)) {
        qWarning() << "WavPlayer: format not supported";
        return;
    }

    m_buffer = new QBuffer(this);
    m_buffer->setData(pcmData);
    m_buffer->open(QIODevice::ReadOnly);

    m_audioOutput = new QAudioOutput(format, this);
    connect(m_audioOutput, &QAudioOutput::stateChanged,
            this, &WavPlayer::slot_stateChanged);

    m_audioOutput->start(m_buffer);
    m_isPlaying = true;
}

void WavPlayer::stop()
{
    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
    if (m_buffer) {
        m_buffer->close();
        delete m_buffer;
        m_buffer = nullptr;
    }
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    m_isPlaying = false;
}

void WavPlayer::slot_stateChanged(QAudio::State state)
{
    if (state == QAudio::IdleState) {
        m_isPlaying = false;
        Q_EMIT sig_playbackFinished();
    }
}
