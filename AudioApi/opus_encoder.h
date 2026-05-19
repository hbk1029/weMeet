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
    void sig_encodedData(const char* data, int len);

public slots:
    void slot_encode(const char* pcmData, int len);

private:
    ::OpusEncoder* m_pEncoder;
};

#endif // OPUS_ENCODER_H
