#include "audio_dsp.h"
#include <QDebug>

AudioDSP::AudioDSP(QObject* parent)
    : QObject(parent)
{
}

AudioDSP::~AudioDSP()
{
    destroy();
}

bool AudioDSP::init(int sampleRate, int frameSamples)
{
    if (m_isInit) {
        qWarning() << "[AudioDSP] 已初始化，先调用 destroy() 再重新 init";
        return false;
    }

    m_sampleRate   = sampleRate;
    m_frameSamples = frameSamples;

    // 回声尾长度：200ms（覆盖中小房间，公式: tail_ms * sampleRate / 1000）
    int filterLength = 200 * sampleRate / 1000;

    // 1. 创建回声消除器
    m_pEchoState = speex_echo_state_init(frameSamples, filterLength);
    if (!m_pEchoState) {
        qWarning() << "[AudioDSP] speex_echo_state_init 失败";
        return false;
    }

    // 设置 AEC 采样率
    speex_echo_ctl(m_pEchoState, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);

    // 2. 创建预处理器（降噪 + AGC + 残余回声抑制）
    m_pPreState = speex_preprocess_state_init(frameSamples, sampleRate);
    if (!m_pPreState) {
        qWarning() << "[AudioDSP] speex_preprocess_state_init 失败";
        speex_echo_state_destroy(m_pEchoState);
        m_pEchoState = nullptr;
        return false;
    }

    // --- 配置预处理器参数 ---

    // 降噪：15dB 衰减
    int enable = 1;
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_DENOISE, &enable);
    int noiseSuppress = -15;
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress);

    // AGC：自动增益控制
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_AGC, &enable);
    float agcLevel = 1.0f;
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agcLevel);
    int agcMaxGain = 30;  // 最大增益 30dB
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &agcMaxGain);
    int agcIncrement = 12;   // 增益上升 12dB/s
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &agcIncrement);
    int agcDecrement = -40;  // 增益下降 40dB/s（快速压制突发大音量）
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &agcDecrement);

    // 残余回声抑制：关联回声消除器，二次清理 AEC 漏掉的回声
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_ECHO_STATE, m_pEchoState);
    int echoSuppress = -20;   // 残余回声 20dB 衰减
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &echoSuppress);
    int echoSuppressActive = -15;  // 近端讲话时轻度抑制（保留双讲）
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &echoSuppressActive);

    // 关闭 Speex 自带的 VAD（我们已有 WebRTC VAD，避免双重检测）
    int vadOff = 0;
    speex_preprocess_ctl(m_pPreState, SPEEX_PREPROCESS_SET_VAD, &vadOff);

    m_isInit = true;
    qDebug() << "[AudioDSP] 初始化完成 sr=" << sampleRate << "frame=" << frameSamples
             << "tail=" << filterLength << "samples";
    return true;
}

void AudioDSP::processCapture(spx_int16_t* pcm)
{
    if (!m_isInit || !pcm) return;

    // Step 1: AEC 回声消除（原地输出）
    // speex_echo_capture 使用内部 playback 缓冲区，延迟 2 帧
    speex_echo_capture(m_pEchoState, pcm, pcm);

    // Step 2: 预处理器（降噪 + 残余回声抑制 + AGC），原地修改
    speex_preprocess_run(m_pPreState, pcm);
}

void AudioDSP::processPlayback(const spx_int16_t* pcm)
{
    if (!m_isInit || !pcm) return;

    // 将远端播放数据喂给 AEC 作为回声参考
    speex_echo_playback(m_pEchoState, pcm);
}

void AudioDSP::reset()
{
    if (!m_isInit) return;

    speex_echo_state_reset(m_pEchoState);
    qDebug() << "[AudioDSP] DSP 状态已重置";
}

void AudioDSP::destroy()
{
    if (m_pPreState) {
        speex_preprocess_state_destroy(m_pPreState);
        m_pPreState = nullptr;
    }
    if (m_pEchoState) {
        speex_echo_state_destroy(m_pEchoState);
        m_pEchoState = nullptr;
    }
    m_isInit = false;
    m_frameSamples = 0;
    m_sampleRate = 0;
    qDebug() << "[AudioDSP] DSP 资源已释放";
}
