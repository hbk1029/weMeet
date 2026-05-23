// WebRTC SPL (Signal Processing Library) minimal implementations
// 只实现 VAD 16kHz 路径实际需要的函数，替代 MSVC 编译的 signalProcess.lib

#include "signal_processing_library.h"
#include <stdint.h>
#include <string.h>

// 初始化函数指针（VAD 路径下未使用复杂处理函数，空实现即可）
void WebRtcSpl_Init(void)
{
}

// 32位除以16位定点除法，Q格式保留
// 参考 WebRTC division_operations.c 实现
int32_t WebRtcSpl_DivW32W16(int32_t num, int16_t den)
{
    if (den != 0) {
        return num / den;
    } else {
        return (num > 0) ? WEBRTC_SPL_WORD32_MAX : WEBRTC_SPL_WORD32_MIN;
    }
}

// 信号能量计算：返回平方和，同时输出归一化移位量
int32_t WebRtcSpl_Energy(int16_t* vector, int vector_length, int* scale_factor)
{
    int32_t en = 0;
    int i;
    int scaling = 0;

    // 使用 32-bit 累加避免溢出
    int32_t en32 = 0;
    for (i = 0; i < vector_length; i++) {
        en32 += (int32_t)vector[i] * (int32_t)vector[i];
    }

    // 归一化：左移直到最高位为 1（或达到最大值）
    if (en32 == 0) {
        *scale_factor = 0;
        return 0;
    }

    // 计算使 en32 的绝对值 <= WEBRTC_SPL_WORD32_MAX / 2 所需的右移量
    while (en32 > WEBRTC_SPL_WORD32_MAX / 2) {
        en32 >>= 1;
        scaling++;
    }
    // 然后左移到接近 WEBRTC_SPL_WORD32_MAX
    while (en32 < WEBRTC_SPL_WORD32_MAX / 4 && scaling > 0) {
        en32 <<= 1;
        scaling--;
    }

    en = (int32_t)en32;
    *scale_factor = scaling;
    return en;
}

// 48kHz->8kHz 重采样状态初始化
void WebRtcSpl_ResetResample48khzTo8khz(WebRtcSpl_State48khzTo8khz* state)
{
    memset(state, 0, sizeof(*state));
}

// 48kHz->8kHz 重采样（当前 16kHz VAD 路径不会调用，但链接器需要符号）
void WebRtcSpl_Resample48khzTo8khz(const int16_t* in, int16_t* out,
                                    WebRtcSpl_State48khzTo8khz* state,
                                    int32_t* tmpmem)
{
    (void)in;
    (void)out;
    (void)state;
    (void)tmpmem;
    // 16kHz VAD 路径从不调用此函数
}
