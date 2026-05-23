#include "vad_wrapper.h"
#include "vad/webrtc_vad.h"
#include <QDebug>
#include <vector>
#include <algorithm>

extern "C" {
#include "vad/typedefs.h"
}

VadWrapper::VadWrapper(QObject *parent)
    : QObject(parent)
    , m_handle(nullptr)
    , m_initialized(false)
{
}

VadWrapper::~VadWrapper()
{
    if (m_handle) {
        WebRtcVad_Free(m_handle);
        m_handle = nullptr;
    }
}

bool VadWrapper::init(int mode)
{
    if (m_initialized) return true;

    if (WebRtcVad_Create(&m_handle) != 0) {
        qWarning() << "VadWrapper: WebRtcVad_Create failed";
        return false;
    }

    if (WebRtcVad_Init(m_handle) != 0) {
        qWarning() << "VadWrapper: WebRtcVad_Init failed";
        WebRtcVad_Free(m_handle);
        m_handle = nullptr;
        return false;
    }

    if (WebRtcVad_set_mode(m_handle, mode) != 0) {
        qWarning() << "VadWrapper: WebRtcVad_set_mode failed";
        WebRtcVad_Free(m_handle);
        m_handle = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}

bool VadWrapper::isVoice(int16_t* pcmData, int sampleRate, int samples)
{
    if (!m_initialized || !m_handle || !pcmData) return false;

    int fs = sampleRate;
    int frameLength = samples;

    // 48kHz 不支持，需降采样至 16kHz
    if (sampleRate == 48000) {
        fs = 16000;
        int downSamples = samples / 3;
        if (downSamples <= 0) return false;
        std::vector<int16_t> downsampled(downSamples);
        for (int i = 0; i < downSamples; ++i) {
            downsampled[i] = pcmData[i * 3];
        }
        frameLength = downSamples;
        int ret = WebRtcVad_Process(m_handle, fs, downsampled.data(), frameLength);
        return (ret == 1);
    }

    // 验证采样率和帧长是否合法
    if (WebRtcVad_ValidRateAndFrameLength(fs, frameLength) != 0) {
        return false;
    }

    int ret = WebRtcVad_Process(m_handle, fs, pcmData, frameLength);
    return (ret == 1);
}
