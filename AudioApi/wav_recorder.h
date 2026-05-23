#ifndef WAV_RECORDER_H
#define WAV_RECORDER_H

#include <QObject>
#include <QFile>
#include <QMutex>

class WavRecorder : public QObject
{
    Q_OBJECT
public:
    explicit WavRecorder(QObject *parent = nullptr);
    ~WavRecorder();

    bool startRecording(const QString& filePath, int sampleRate = 48000,
                        int channels = 1, int bitsPerSample = 16);
    void stopRecording();
    bool isRecording() const { return m_isRecording; }

    // 线程安全写入 PCM 数据
    void writePcmData(const char* data, int len);

private:
    void writeWavHeader();

    QFile m_file;
    QMutex m_mutex;
    bool m_isRecording;
    int m_sampleRate;
    int m_channels;
    int m_bitsPerSample;
    int m_dataSize;
};

#endif // WAV_RECORDER_H
