#ifndef WAV_PLAYER_H
#define WAV_PLAYER_H

#include <QObject>
#include <QFile>
#include <QAudioOutput>
#include <QBuffer>

class WavPlayer : public QObject
{
    Q_OBJECT
public:
    explicit WavPlayer(QObject *parent = nullptr);
    ~WavPlayer();

    void play(const QString& filePath);
    void stop();
    bool isPlaying() const { return m_isPlaying; }

signals:
    void sig_playbackFinished();

private slots:
    void slot_stateChanged(QAudio::State state);

private:
    bool m_isPlaying;
    QFile* m_file;
    QAudioOutput* m_audioOutput;
    QBuffer* m_buffer;
};

#endif // WAV_PLAYER_H
