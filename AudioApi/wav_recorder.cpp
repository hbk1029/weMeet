#include "wav_recorder.h"
#include <QDebug>

WavRecorder::WavRecorder(QObject *parent)
    : QObject(parent)
    , m_isRecording(false)
    , m_sampleRate(48000)
    , m_channels(1)
    , m_bitsPerSample(16)
    , m_dataSize(0)
{
}

WavRecorder::~WavRecorder()
{
    if (m_isRecording) {
        stopRecording();
    }
}

bool WavRecorder::startRecording(const QString& filePath, int sampleRate,
                                  int channels, int bitsPerSample)
{
    if (m_isRecording) return false;

    m_sampleRate = sampleRate;
    m_channels = channels;
    m_bitsPerSample = bitsPerSample;
    m_dataSize = 0;

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly)) {
        qWarning() << "WavRecorder: failed to open" << filePath;
        return false;
    }

    writeWavHeader();
    m_isRecording = true;
    return true;
}

void WavRecorder::stopRecording()
{
    if (!m_isRecording) return;
    m_isRecording = false;

    QMutexLocker locker(&m_mutex);

    // 回写 WAV 头部的总大小字段
    if (m_file.isOpen()) {
        int fileSize = 36 + m_dataSize;
        int byteRate = m_sampleRate * m_channels * m_bitsPerSample / 8;
        int blockAlign = m_channels * m_bitsPerSample / 8;

        m_file.seek(4);
        m_file.write(reinterpret_cast<const char*>(&fileSize), 4);

        m_file.seek(28);
        m_file.write(reinterpret_cast<const char*>(&byteRate), 4);

        m_file.seek(32);
        m_file.write(reinterpret_cast<const char*>(&blockAlign), 2);

        m_file.seek(40);
        m_file.write(reinterpret_cast<const char*>(&m_dataSize), 4);

        m_file.close();
    }
}

void WavRecorder::writePcmData(const char* data, int len)
{
    if (!m_isRecording || !data || len <= 0) return;

    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.write(data, len);
        m_dataSize += len;
    }
}

void WavRecorder::writeWavHeader()
{
    // RIFF chunk
    m_file.write("RIFF", 4);
    int chunkSize = 0; // 占位，stopRecording 时回写
    m_file.write(reinterpret_cast<const char*>(&chunkSize), 4);
    m_file.write("WAVE", 4);

    // fmt chunk
    m_file.write("fmt ", 4);
    int subchunk1Size = 16; // PCM
    m_file.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    short audioFormat = 1; // PCM
    m_file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    short numChannels = static_cast<short>(m_channels);
    m_file.write(reinterpret_cast<const char*>(&numChannels), 2);
    int sampleRate = m_sampleRate;
    m_file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    int byteRate = 0; // 占位
    m_file.write(reinterpret_cast<const char*>(&byteRate), 4);
    short blockAlign = 0; // 占位
    m_file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    short bitsPerSample = static_cast<short>(m_bitsPerSample);
    m_file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    m_file.write("data", 4);
    int dataSize = 0; // 占位
    m_file.write(reinterpret_cast<const char*>(&dataSize), 4);
}
