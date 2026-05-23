#include "sdl_audio_read.h"
#include "audio_dsp.h"
#include "tracelog.h"
#ifdef USE_VAD
#include "vad_wrapper.h"
#endif

#ifdef USE_SDL2

SDLAudioRead::SDLAudioRead(QObject *parent)
    : QObject(parent)
    , m_dev(0)
    , m_isOpen(false)
#ifdef USE_VAD
    , m_pVad(nullptr)
#endif
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug() << "SDLAudioRead: Failed to initialize SDL:" << SDL_GetError();
        return;
    }

#ifdef USE_VAD
    m_pVad = new VadWrapper(this);
    if (!m_pVad->init(0)) {  // VAD 模式 0：最低激进度，避免过滤过多语音帧
        qWarning() << "SDLAudioRead: VAD init failed, VAD disabled";
        delete m_pVad;
        m_pVad = nullptr;
    }
#endif

    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_memset(&desiredSpec, 0, sizeof(desiredSpec));
    desiredSpec.freq = 48000;
    desiredSpec.format = AUDIO_S16LSB;
    desiredSpec.channels = 1;
    desiredSpec.samples = 960;
    desiredSpec.callback = audioCallback;
    desiredSpec.userdata = this;

    m_dev = SDL_OpenAudioDevice(NULL, 1, &desiredSpec, &obtainedSpec, 0);
    if (m_dev == 0) {
        qDebug() << "SDLAudioRead: Failed to open audio device:" << SDL_GetError();
        SDL_Quit();
        return;
    }
}

SDLAudioRead::~SDLAudioRead()
{
    if (m_isOpen) {
        SDL_PauseAudioDevice(m_dev, 1);
    }
    if (m_dev != 0) {
        SDL_CloseAudioDevice(m_dev);
    }
    SDL_Quit();
}

void SDLAudioRead::audioCallback(void *userdata, Uint8 *stream, int len)
{
    SDLAudioRead *audio = (SDLAudioRead *)userdata;
    if (len < 1920) return;

    static int cbCount = 0, vadPass = 0, vadSkip = 0;
    cbCount++;
    if (cbCount <= 3) TRACE("AUDIO_CAP cb #%d len=%d thread=%lu", cbCount, len, GetCurrentThreadId());

    // DSP 处理：AEC 回声消除 + 降噪 + AGC（原地修改 PCM）
    if (audio->m_pDSP && audio->m_pDSP->isInit()) {
        audio->m_pDSP->processCapture(reinterpret_cast<spx_int16_t*>(stream));
    }

#ifdef USE_VAD
    // VAD 检测：静音帧不发送
    if (audio->m_pVad) {
        int16_t* pcm = reinterpret_cast<int16_t*>(stream);
        int samples = len / 2; // 16-bit = 2 bytes per sample
        if (!audio->m_pVad->isVoice(pcm, 48000, samples)) {
            vadSkip++;
            if (cbCount % 100 == 0)
                qDebug() << "[AUDIO_CAP] cb=" << cbCount << " vadPass=" << vadPass << " vadSkip=" << vadSkip << " STATUS=SKIP(silence)";
            return; // 静音，跳过
        }
        vadPass++;
    }
#endif

    if (cbCount % 100 == 0)
        qDebug() << "[AUDIO_CAP] cb=" << cbCount << " len=" << len << " vadPass=" << vadPass << " vadSkip=" << vadSkip << " STATUS=SEND";

    QByteArray sendBuffer((char*)stream, len);
    Q_EMIT audio->SIG_sendAudioFrame(sendBuffer);
}

void SDLAudioRead::slot_openAudio()
{
    if (m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 0);
    m_isOpen = true;
}

void SDLAudioRead::slot_closeAudio()
{
    if (!m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 1);
    m_isOpen = false;
}

#endif // USE_SDL2
