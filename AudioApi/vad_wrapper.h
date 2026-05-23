#ifndef VAD_WRAPPER_H
#define VAD_WRAPPER_H

#include <QObject>

struct WebRtcVadInst;
typedef WebRtcVadInst VadInst;

class VadWrapper : public QObject
{
    Q_OBJECT
public:
    explicit VadWrapper(QObject *parent = nullptr);
    ~VadWrapper();

    // mode: 0=高质量 1=中等 2=激进 3=极激进
    bool init(int mode = 1);
    // pcmData: 16-bit PCM（非 const 仅为兼容 WebRTC API，实际不修改）
    // 返回 true=语音, false=静音
    bool isVoice(int16_t* pcmData, int sampleRate, int samples);

private:
    VadInst* m_handle;
    bool m_initialized;
};

#endif // VAD_WRAPPER_H
