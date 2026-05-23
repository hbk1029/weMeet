#pragma once
/* AudioDSP — SpeexDSP 音频处理封装（AEC回声消除 + 降噪 + AGC）
 *
 * 依赖：libspeexdsp.dll（speex_echo.h / speex_preprocess.h）
 * 调用流程：
 *   1. init(48000, 960) → 创建 AEC + 预处理状态
 *   2. 每帧: processPlayback(farPcm) → 喂回声参考
 *   3. 每帧: processCapture(nearPcm) → AEC + 降噪 + AGC（原地修改）
 *   4. 退出时: destroy() → 释放资源
 */

#include <QObject>
#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

class AudioDSP : public QObject
{
    Q_OBJECT
public:
    explicit AudioDSP(QObject* parent = nullptr);
    ~AudioDSP() override;

    /* 初始化 DSP 引擎
     * @param sampleRate   采样率（Hz），如 48000
     * @param frameSamples 每帧采样数，如 960（48kHz 下 20ms）
     * @return true 成功, false 失败
     */
    bool init(int sampleRate, int frameSamples);

    /* 处理麦克风采集路径（原地修改）
     * 执行：AEC 回声消除 → 降噪 → AGC
     * @param pcm 输入/输出 PCM 数据（spx_int16_t[frameSamples]）
     */
    void processCapture(spx_int16_t* pcm);

    /* 喂入远端音频作为回声参考
     * @param pcm 远端播放的 PCM 数据（spx_int16_t[frameSamples]）
     */
    void processPlayback(const spx_int16_t* pcm);

    /* 重置 DSP 状态（设备切换后调用） */
    void reset();

    /* 释放 DSP 资源 */
    void destroy();

    /* 是否已初始化 */
    bool isInit() const { return m_isInit; }

private:
    SpeexEchoState*       m_pEchoState = nullptr;
    SpeexPreprocessState* m_pPreState  = nullptr;
    int m_frameSamples = 0;
    int m_sampleRate   = 0;
    bool m_isInit      = false;
};
