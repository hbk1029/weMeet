#include "sdl_audio_write.h"
#include "audio_dsp.h"
#include "tracelog.h"

#ifdef USE_SDL2

SDLAudioWrite::SDLAudioWrite(QObject *parent)
    : QObject(parent)
    , m_dev(0)
    , m_isOpen(false)
    , m_isBuffered(false)
    , m_preBufCount(0)
    , m_pOpusDecoder(nullptr)
{
    // SDL 可能在 SDLAudioRead 中已初始化，检查避免重复
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            qDebug() << "SDLAudioWrite: Failed to initialize SDL:" << SDL_GetError();
            return;
        }
    }

    SDL_AudioSpec wantedSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_zero(wantedSpec);
    wantedSpec.freq = 48000;
    wantedSpec.format = AUDIO_S16LSB;
    wantedSpec.channels = 1;
    wantedSpec.samples = 960;
    wantedSpec.callback = audioCallback;
    wantedSpec.userdata = this;

    m_dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
    if (m_dev == 0) {
        qDebug() << "SDLAudioWrite: Failed to open audio device:" << SDL_GetError();
        return;
    }

    // 创建 Opus 解码器，解码后 PCM 直接入队播放
    m_pOpusDecoder = new OpusDecoder(this);
    connect(m_pOpusDecoder, &OpusDecoder::sig_decodedData, this,
            &SDLAudioWrite::slot_playAudioFrame);
}

SDLAudioWrite::~SDLAudioWrite()
{
    if (m_isOpen) {
        SDL_PauseAudioDevice(m_dev, 1);
    }
    if (m_dev != 0) {
        SDL_CloseAudioDevice(m_dev);
    }
    // SDL_Quit() 由 SDLAudioRead 负责，Write 端不调用以避免生命周期冲突
    // m_pOpusDecoder 由 Qt 父子关系自动释放(parent=this)
}

void SDLAudioWrite::audioCallback(void *userdata, Uint8 *stream, int len)
{
    SDLAudioWrite *audio = (SDLAudioWrite *)userdata;
    memset(stream, 0, len);

    static int playCbCount = 0; playCbCount++;
    int qsz = 0;
    {
        std::lock_guard<std::mutex> lck(audio->m_mutex);
        qsz = (int)audio->m_audioQueue.size();

        // 预缓冲阶段：缓冲不足时不播放，用静音填充避免爆破音
        if (!audio->m_isBuffered) {
            if (qsz >= kPreBufTarget) {
                audio->m_isBuffered = true;
            } else {
                if (playCbCount <= 3 || playCbCount % 100 == 0)
                    TRACE("AUDIO_PLAY cb #%d prebuf=%d/%d", playCbCount, qsz, kPreBufTarget);
                return;  // 静音，等待缓冲
            }
        }

        if (!audio->m_audioQueue.empty()) {
            QByteArray recvBuffer = audio->m_audioQueue.front();
            audio->m_audioQueue.pop_front();
            // 防止解码器返回异常长度导致缓冲区溢出
            int mixLen = recvBuffer.size();
            if (mixLen > len) mixLen = len;
            SDL_MixAudioFormat(stream, (Uint8*)recvBuffer.data(), AUDIO_S16LSB,
                              mixLen, SDL_MIX_MAXVOLUME);
            // 将实际播放的远端 PCM 喂给 AEC 作为回声参考
            if (audio->m_pDSP && audio->m_pDSP->isInit()) {
                audio->m_pDSP->processPlayback(reinterpret_cast<const spx_int16_t*>(recvBuffer.constData()));
            }
        }
    }
    if (playCbCount <= 3 || playCbCount % 100 == 0)
        TRACE("AUDIO_PLAY cb #%d queue=%d", playCbCount, qsz);
}

void SDLAudioWrite::slot_openAudio()
{
    if (m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 0);
    m_isOpen = true;
}

void SDLAudioWrite::slot_closeAudio()
{
    if (!m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 1);
    m_isOpen = false;
    // 重置预缓冲状态，下次打开时重新缓冲
    m_isBuffered = false;
    m_preBufCount = 0;
    { std::lock_guard<std::mutex> lck(m_mutex); m_audioQueue.clear(); }
}

void SDLAudioWrite::slot_net_rx(const char* data, int len)
{
    if (!data || len <= 0) return;
    static int rxCount = 0; rxCount++;
    if (rxCount <= 3 || rxCount % 100 == 0) {
        int qsz;
        { std::lock_guard<std::mutex> lck(m_mutex); qsz = (int)m_audioQueue.size(); }
        TRACE("AUDIO_RX #%d len=%d queue=%d thread=%lu", rxCount, len, qsz, GetCurrentThreadId());
    }
    if (m_pOpusDecoder) {
        m_pOpusDecoder->slot_decode((const unsigned char*)data, len);
    }
}

void SDLAudioWrite::slot_playAudioFrame(QByteArray recvBuffer)
{
    static int dbgCount = 0; dbgCount++;
    if (dbgCount <= 5)
        TRACE("AUDIO_ENQ_ENTER #%d isOpen=%d size=%d", dbgCount, (int)m_isOpen, recvBuffer.size());
    if (!m_isOpen) return;
    std::lock_guard<std::mutex> lck(m_mutex);
    // 队列无上限会导致延迟持续累积，超过阈值丢弃最旧帧
    while ((int)m_audioQueue.size() >= kMaxQueueSize) {
        m_audioQueue.pop_front();
    }
    m_audioQueue.emplace_back(recvBuffer);
    static int playRxCount = 0; playRxCount++;
    if (playRxCount <= 3 || playRxCount % 100 == 0)
        TRACE("AUDIO_ENQ #%d queue=%d", playRxCount, (int)m_audioQueue.size());
}

#endif // USE_SDL2
