#ifndef AUDIO_READ_H
#define AUDIO_READ_H

#include <QObject>
#include"audio_common.h"
#include<QTimer>
class Audio_Read : public QObject
{
    Q_OBJECT
public:
    explicit Audio_Read(QObject *parent = nullptr);
    ~Audio_Read();
    enum audio_state{ _Stop , _Record , _Pause };
signals:
    void sig_audioFrame(const char* data, int len);
public slots:
    void slot_startRecord();
    void slot_pauseRecord();
    void slot_readMore();

private:
    QAudioFormat    m_format;
    QTimer*         m_pTimer;
    QAudioInput*    m_pAudioIn;
    QIODevice*      m_pBufferIn;
    int             m_recordState;
    //Speex 编码
    SpeexBits       m_bitsEnc;
    void*           m_pEncState;
};

#endif // AUDIO_READ_H
