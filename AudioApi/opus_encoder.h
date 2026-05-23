#ifndef OPUS_ENCODER_H
#define OPUS_ENCODER_H

#include <QObject>

extern "C" {
#include <opus/opus.h>
}

class OpusEncoder : public QObject
{
    Q_OBJECT
public:
    explicit OpusEncoder(QObject *parent = nullptr);
    ~OpusEncoder();

signals:
    void sig_encodedData(const QByteArray& data);  // QByteArray 跨线程安全深拷贝

public slots:
    void slot_encode(const char* pcmData, int len);

private:
    ::OpusEncoder* m_pEncoder;
    QByteArray m_encodedBuffer;  // 编码缓冲区，生命周期与对象一致，跨线程信号安全
};

#endif // OPUS_ENCODER_H
