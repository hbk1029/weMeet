#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

#include <QThread>
#include <QImage>
#include <QString>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/time.h"
}

class FFmpegDecoder : public QThread
{
    Q_OBJECT
public:
    explicit FFmpegDecoder(const QString& filePath, QObject *parent = nullptr);
    ~FFmpegDecoder();

    void stop();

signals:
    void SIG_GetOneImage(QImage image);

protected:
    void run() override;

private:
    QString m_fileName;
    bool m_stop;
};

#endif // FFMPEG_DECODER_H
