# 音视频引擎升级 — 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将视频会议系统从 MVP 升级到生产级音视频引擎：SDL2 音频 I/O + OpenGL 视频渲染 + PTS 时间同步 + 编码优化 + 网络包协议

**Architecture:** 保留现有 Qt Audio / QPainter / Speex 路径，用 feature flag（`USE_SDL2`, `USE_OPENGL`）包裹新路径。新旧代码分支共存，可一键回退。每个步骤独立可测试。

**Tech Stack:** C++11, Qt 5.15.2 (MinGW 32-bit), FFmpeg 4.2.2, OpenCV 4.2.0, SDL2 2.0.10, Opus 1.4, OpenGL (Qt 内置)

**现状分析（关键）：**
- `myfacedetect.h/cpp` ✅ 已存在（Haar Cascade 人脸+眼睛双重检测）
- `h264_encoder.h/cpp` ✅ 已存在（libx264 编码，superfast+zerolatency）
- `opus_encoder.h/cpp` + `opus_decoder.h/cpp` ✅ 已存在并工作
- `ffmpeg_decoder.h/cpp` + `ffmpeg_player.h/cpp` ⚠️ 已存在但只支持文件播放，未接入网络管线
- `video_read.cpp` ✅ 已集成人脸检测+萌拍+H.264编码
- `video_write.cpp` ✅ 已集成 H.264 解码+QPainter 渲染
- `audio_read.cpp` ✅ 已用 Qt QAudioInput + Opus
- `audio_write.cpp` ✅ 已用 Qt QAudioOutput + Opus 解码
- SDL2 音频 I/O ❌ 待新建
- OpenGL 渲染 ❌ 待新建
- PTS 时间同步 ❌ 待实现
- 视频网络包协议 ❌ 待新增

---

### Task 1: 新建 SDL2 音频采集类（步骤 06）

**文件：**
- 创建：`AudioApi/sdl_audio_read.h`
- 创建：`AudioApi/sdl_audio_read.cpp`
- 修改：`WeMeetClient.pro`（添加 SDL2 include/lib）
- 修改：`AudioApi/audio_common.h`（添加 `USE_SDL2` feature flag）

- [ ] **Step 1: 复制 SDL2 库文件到项目**

```bash
cp -r "D:/Projects/VideoProject/Video-Better/05-音频播放/SDL2-2.0.10/" "D:/Projects/dev-projects/cpp/qt/demo/WeMeetClient/SDL2-2.0.10/"
```

- [ ] **Step 2: 修改 WeMeetClient.pro 添加 SDL2 依赖**

在 `WeMeetClient.pro` 的 `INCLUDEPATH` 块追加：
```qmake
INCLUDEPATH += $$PWD/SDL2-2.0.10/include
```
在 `LIBS` 块追加：
```qmake
LIBS += $$PWD/SDL2-2.0.10/lib/x86/SDL2.lib
```

- [ ] **Step 3: 修改 audio_common.h 添加 USE_SDL2 flag**

在 `AudioApi/audio_common.h` 中 `#define USE_OPUS` 之后添加：
```cpp
// 注释此行回退到 Qt QAudioInput/QAudioOutput
#define USE_SDL2
```

- [ ] **Step 4: 创建 sdl_audio_read.h**

```cpp
#ifndef SDL_AUDIO_READ_H
#define SDL_AUDIO_READ_H

#include <QObject>
#include <QByteArray>
#include <mutex>

extern "C" {
#include <SDL.h>
}

class SDLAudioRead : public QObject
{
    Q_OBJECT
public:
    explicit SDLAudioRead(QObject *parent = nullptr);
    ~SDLAudioRead();

signals:
    void SIG_sendAudioFrame(QByteArray data);

public slots:
    void slot_openAudio();
    void slot_closeAudio();

private:
    static void audioCallback(void *userdata, Uint8 *stream, int len);

    SDL_AudioDeviceID m_dev;
    bool m_isOpen;
};

#endif // SDL_AUDIO_READ_H
```

- [ ] **Step 5: 创建 sdl_audio_read.cpp**

```cpp
#include "sdl_audio_read.h"
#include <QDebug>

SDLAudioRead::SDLAudioRead(QObject *parent)
    : QObject(parent), m_dev(0), m_isOpen(false)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug() << "[SDL] Failed to init SDL:" << SDL_GetError();
        return;
    }

    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_memset(&desiredSpec, 0, sizeof(desiredSpec));
    desiredSpec.freq = 48000;
    desiredSpec.format = AUDIO_S16LSB;
    desiredSpec.channels = 1;
    desiredSpec.samples = 960;          // 20ms per callback @ 48kHz
    desiredSpec.callback = audioCallback;
    desiredSpec.userdata = this;

    m_dev = SDL_OpenAudioDevice(NULL, 1, &desiredSpec, &obtainedSpec, 0);
    if (m_dev == 0) {
        qDebug() << "[SDL] Failed to open audio device:" << SDL_GetError();
        SDL_Quit();
        return;
    }
    qDebug() << "[SDL] Audio capture device opened, freq=" << obtainedSpec.freq;
}

SDLAudioRead::~SDLAudioRead()
{
    slot_closeAudio();
    if (m_dev != 0) {
        SDL_CloseAudioDevice(m_dev);
    }
    SDL_Quit();
}

void SDLAudioRead::audioCallback(void *userdata, Uint8 *stream, int len)
{
    SDLAudioRead *audio = (SDLAudioRead *)userdata;
    if (len < 1920) return;
    QByteArray sendBuffer((char*)stream, len);
    Q_EMIT audio->SIG_sendAudioFrame(sendBuffer);
}

void SDLAudioRead::slot_openAudio()
{
    if (m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 0);
    m_isOpen = true;
    qDebug() << "[SDL] Audio capture started";
}

void SDLAudioRead::slot_closeAudio()
{
    if (!m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 1);
    m_isOpen = false;
    qDebug() << "[SDL] Audio capture stopped";
}
```

- [ ] **Step 6: 编译验证**

```bash
export PATH="/d/SDKs/Qt/5.15.2/mingw81_32/bin:/d/SDKs/Qt/Tools/mingw810_32/bin:$PATH"
cd "D:/Projects/dev-projects/cpp/qt/demo/WeMeetClient"
qmake WeMeetClient.pro -spec win32-g++ "CONFIG+=release" && mingw32-make -f Makefile.Release
```

成功标准：`sdl_audio_read.obj` 编译通过，无链接错误。

---

### Task 2: 新建 SDL2 音频播放类（步骤 05）

**文件：**
- 创建：`AudioApi/sdl_audio_write.h`
- 创建：`AudioApi/sdl_audio_write.cpp`

- [ ] **Step 1: 创建 sdl_audio_write.h**

```cpp
#ifndef SDL_AUDIO_WRITE_H
#define SDL_AUDIO_WRITE_H

#include <QObject>
#include <QByteArray>
#include <list>
#include <mutex>

extern "C" {
#include <SDL.h>
}

class SDLAudioWrite : public QObject
{
    Q_OBJECT
public:
    explicit SDLAudioWrite(QObject *parent = nullptr);
    ~SDLAudioWrite();

public slots:
    void slot_openAudio();
    void slot_closeAudio();
    void slot_playAudioFrame(QByteArray recvBuffer);

private:
    static void audioCallback(void *userdata, Uint8 *stream, int len);

    SDL_AudioDeviceID m_dev;
    bool m_isOpen;
    std::list<QByteArray> m_audioQueue;
    std::mutex m_mutex;
};

#endif // SDL_AUDIO_WRITE_H
```

- [ ] **Step 2: 创建 sdl_audio_write.cpp**

```cpp
#include "sdl_audio_write.h"
#include <QDebug>

SDLAudioWrite::SDLAudioWrite(QObject *parent)
    : QObject(parent), m_dev(0), m_isOpen(false)
{
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            qDebug() << "[SDL] Write: Failed to init SDL:" << SDL_GetError();
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
        qDebug() << "[SDL] Write: Failed to open audio device:" << SDL_GetError();
        SDL_Quit();
        return;
    }
    qDebug() << "[SDL] Audio playback device opened, freq=" << obtainedSpec.freq;
}

SDLAudioWrite::~SDLAudioWrite()
{
    slot_closeAudio();
    if (m_dev != 0) {
        SDL_CloseAudioDevice(m_dev);
    }
}

void SDLAudioWrite::audioCallback(void *userdata, Uint8 *stream, int len)
{
    SDLAudioWrite *audio = (SDLAudioWrite *)userdata;
    memset(stream, 0, len);

    if (!audio->m_audioQueue.empty()) {
        std::lock_guard<std::mutex> lck(audio->m_mutex);
        if (!audio->m_audioQueue.empty()) {
            QByteArray recvBuffer = audio->m_audioQueue.front();
            audio->m_audioQueue.pop_front();
            SDL_MixAudioFormat(stream, (uint8_t*)recvBuffer.data(),
                               AUDIO_S16LSB, recvBuffer.size(), 100);
        }
    }
}

void SDLAudioWrite::slot_openAudio()
{
    if (m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 0);
    m_isOpen = true;
    qDebug() << "[SDL] Audio playback started";
}

void SDLAudioWrite::slot_closeAudio()
{
    if (!m_isOpen) return;
    SDL_PauseAudioDevice(m_dev, 1);
    m_isOpen = false;
    qDebug() << "[SDL] Audio playback stopped";
}

void SDLAudioWrite::slot_playAudioFrame(QByteArray recvBuffer)
{
    if (!m_isOpen) return;
    std::lock_guard<std::mutex> lck(m_mutex);
    m_audioQueue.emplace_back(recvBuffer);
}
```

- [ ] **Step 3: 更新 WeMeetClient.pro 添加新文件**

在 `SOURCES` 块追加：
```qmake
    AudioApi/sdl_audio_read.cpp \
    AudioApi/sdl_audio_write.cpp
```
在 `HEADERS` 块追加：
```qmake
    AudioApi/sdl_audio_read.h \
    AudioApi/sdl_audio_write.h
```

- [ ] **Step 4: 编译验证**

```bash
export PATH="/d/SDKs/Qt/5.15.2/mingw81_32/bin:/d/SDKs/Qt/Tools/mingw810_32/bin:$PATH"
cd "D:/Projects/dev-projects/cpp/qt/demo/WeMeetClient"
qmake WeMeetClient.pro -spec win32-g++ "CONFIG+=release" && mingw32-make -f Makefile.Release
```

成功标准：编译通过，无链接错误。

---

### Task 3: 集成 SDL2 音频到 meetingdialog（替换 Qt Audio 路径）

**文件：**
- 修改：`meetingdialog.h`
- 修改：`meetingdialog.cpp`
- 修改：`kernel.cpp`（如音频信号路由有变化）

- [ ] **Step 1: 修改 meetingdialog.h 添加 SDL 成员**

在 `meetingdialog.h` 的 `#include` 区域添加（`USE_SDL2` 条件编译）：
```cpp
#ifdef USE_SDL2
#include "AudioApi/sdl_audio_read.h"
#include "AudioApi/sdl_audio_write.h"
#endif
```

在 `private` 成员区域，将现有的 `Audio_Read* m_pAudioRead; Audio_Write* m_pAudioWrite;` 改为：
```cpp
#ifdef USE_SDL2
    SDLAudioRead*  m_pSDLAudioRead;
    SDLAudioWrite* m_pSDLAudioWrite;
#else
    Audio_Read*  m_pAudioRead;
    Audio_Write* m_pAudioWrite;
#endif
```

- [ ] **Step 2: 修改 meetingdialog 构造函数和析构函数**

在构造函数中（`USE_SDL2` 分支）：
```cpp
#ifdef USE_SDL2
    m_pSDLAudioRead = new SDLAudioRead(this);
    m_pSDLAudioWrite = new SDLAudioWrite(this);
    connect(m_pSDLAudioRead, &SDLAudioRead::SIG_sendAudioFrame, this,
            [this](QByteArray data) {
        Q_EMIT sig_meetingAudio(data.data(), data.size());
    });
#else
    // 原有 Qt Audio 初始化代码保持不变
#endif
```

- [ ] **Step 3: 修改麦克风按钮逻辑**

`on_pb_microphone_clicked()` 中，`USE_SDL2` 分支调用 `m_pSDLAudioRead->slot_openAudio/slot_closeAudio`。

- [ ] **Step 4: 修改音频接收逻辑**

在 `kernel.cpp` 或 `meetingdialog.cpp` 中音频数据到达时（网络回调），`USE_SDL2` 分支调用 `m_pSDLAudioWrite->slot_playAudioFrame(QByteArray(data, len))`。

- [ ] **Step 5: 编译验证**

```bash
export PATH="/d/SDKs/Qt/5.15.2/mingw81_32/bin:/d/SDKs/Qt/Tools/mingw810_32/bin:$PATH"
cd "D:/Projects/dev-projects/cpp/qt/demo/WeMeetClient"
qmake WeMeetClient.pro -spec win32-g++ "CONFIG+=release" && mingw32-make -f Makefile.Release
```

成功标准：编译通过。运行后音频采集/播放正常工作，`USE_SDL2` 注释掉后回退到 Qt Audio 正常。

---

### Task 4: 视频 PTS 时间同步（步骤 04）

**文件：**
- 修改：`VideoApi/video_read.cpp`
- 修改：`VideoApi/video_read.h`

- [ ] **Step 1: 在 video_read.h 添加 PTS 相关成员**

```cpp
private:
    int64_t m_startTime;   // 采集起始时间（av_gettime）
    int m_frameCount;      // 已编码帧数
```

- [ ] **Step 2: 修改 video_read.cpp 编码前计算 PTS**

在 `slot_getVideoFrame()` 中 H.264 编码路径，BGR→YUV 之前添加：
```cpp
#ifdef USE_H264
    // PTS 时间戳（微秒）
    if (m_frameCount == 0) {
        m_startTime = av_gettime();
    }
    int64_t pts = av_gettime() - m_startTime;
    // m_pH264Encoder 内部会将此 pts 设到 AVPacket
    m_pH264Encoder->setPts(pts);
    m_frameCount++;
#endif
```

- [ ] **Step 3: 在 h264_encoder.h/cpp 添加 setPts 方法**

```cpp
// h264_encoder.h
public:
    void setPts(int64_t pts);
private:
    int64_t m_pts;

// h264_encoder.cpp
void H264Encoder::setPts(int64_t pts) { m_pts = pts; }

// 在 slot_encode() 中 avcodec_send_frame 之前：
m_pFrame->pts = m_pts;
```

- [ ] **Step 4: 编译验证**

成功标准：H.264 编码帧带上 PTS 时间戳。

---

### Task 5: OpenGL 视频渲染组件（步骤 09）

**文件：**
- 创建：`VideoApi/opengl_render.h`
- 创建：`VideoApi/opengl_render.cpp`
- 修改：`meetingdialog.h/cpp`（集成 OpenGL widget）

- [ ] **Step 1: 创建 opengl_render.h**

```cpp
#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QImage>
#include <QMutex>

class OpenGLRender : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLRender(QWidget *parent = nullptr);

public slots:
    void slot_updateImage(QImage img);
    void slot_clearFrame();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QImage m_image;
    QMutex m_mutex;
    bool m_hasFrame;
};

#endif // OPENGL_RENDER_H
```

- [ ] **Step 2: 创建 opengl_render.cpp**

```cpp
#include "opengl_render.h"
#include <QPainter>

OpenGLRender::OpenGLRender(QWidget *parent)
    : QOpenGLWidget(parent), m_hasFrame(false)
{
    setMinimumSize(160, 120);
}

void OpenGLRender::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.102f, 0.102f, 0.180f, 1.0f);  // #1A1A2E
}

void OpenGLRender::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    QMutexLocker locker(&m_mutex);
    if (!m_hasFrame || m_image.isNull()) {
        QPainter painter(this);
        painter.setPen(QColor(0x66, 0x66, 0x66));
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("等待视频..."));
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QImage scaled = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (width() - scaled.width()) / 2;
    int y = (height() - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
}

void OpenGLRender::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void OpenGLRender::slot_updateImage(QImage img)
{
    QMutexLocker locker(&m_mutex);
    m_image = img;
    m_hasFrame = true;
    locker.unlock();
    update();
}

void OpenGLRender::slot_clearFrame()
{
    QMutexLocker locker(&m_mutex);
    m_image = QImage();
    m_hasFrame = false;
    locker.unlock();
    update();
}
```

- [ ] **Step 3: 编译验证**

成功标准：`opengl_render.obj` 编译通过。

---

### Task 6: 集成 OpenGL 渲染到 meetingdialog（步骤 09 续）

**文件：**
- 修改：`meetingdialog.h`
- 修改：`meetingdialog.cpp` / `meetingdialog.ui`

- [ ] **Step 1: meetingdialog 中用 OpenGLRender 替代 Video_Write**

在 `meetingdialog.h` 中 `#define USE_OPENGL` 包裹：
```cpp
#ifdef USE_OPENGL
#include "VideoApi/opengl_render.h"
#endif

// 成员变量
#ifdef USE_OPENGL
    OpenGLRender* m_pGLRender;        // 对方画面
    OpenGLRender* m_pGLRenderLocal;   // 本地画中画
#endif
```

- [ ] **Step 2: 构造函数中创建 OpenGLRender 实例并布局**

```cpp
#ifdef USE_OPENGL
    m_pGLRender = new OpenGLRender(ui->wdg_video);
    // 添加到 wdg_video 的布局中
    m_pGLRenderLocal = new OpenGLRender(ui->wdg_local_video);
#else
    // 原有 Video_Write 初始化代码保持不变
#endif
```

- [ ] **Step 3: 视频帧信号路由到 slot_updateImage**

网络接收回调中 `USE_OPENGL` 分支调用 `m_pGLRender->slot_updateImage(img)`。

- [ ] **Step 4: 编译验证**

成功标准：编译通过。运行后视频画面用 OpenGL 渲染，注释 `USE_OPENGL` 回退到 QPainter 正常。

---

### Task 7: 编码丢帧策略与缓冲队列（步骤 12）

**文件：**
- 创建：`VideoApi/frame_buffer.h`
- 修改：`VideoApi/video_read.cpp`（采集端丢帧）
- 修改：`VideoApi/video_write.cpp`（接收端缓冲）

- [ ] **Step 1: 创建 frame_buffer.h（环形缓冲）**

```cpp
#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <QMutex>
#include <QQueue>
#include <QByteArray>

struct FrameNode {
    QByteArray data;
    int64_t pts;
};

class FrameBuffer
{
public:
    explicit FrameBuffer(int maxSize = 3) : m_maxSize(maxSize) {}

    void push(const QByteArray& data, int64_t pts) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.size() >= m_maxSize) {
            m_queue.dequeue();  // 丢弃最旧帧
        }
        FrameNode node;
        node.data = data;
        node.pts = pts;
        m_queue.enqueue(node);
    }

    bool pop(QByteArray& data, int64_t& pts) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.isEmpty()) return false;
        FrameNode node = m_queue.dequeue();
        data = node.data;
        pts = node.pts;
        return true;
    }

    void clear() {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
    }

private:
    QQueue<FrameNode> m_queue;
    QMutex m_mutex;
    int m_maxSize;
};

#endif // FRAME_BUFFER_H
```

- [ ] **Step 2: 在 video_read.cpp 编码前丢帧**

```cpp
// slot_getVideoFrame 最开头添加跳帧逻辑
if (m_frameSkipCounter++ % 2 != 0) return;  // 采集30fps，编码15fps
```

- [ ] **Step 3: 在 video_write.cpp 用 FrameBuffer 缓冲接收数据**

```cpp
// video_write.h 添加成员
FrameBuffer m_frameBuf;

// slot_recvVideoFrame 中，解码前先入缓冲
m_frameBuf.push(QByteArray(data, len), pts);
QByteArray frameData;
int64_t framePts;
while (m_frameBuf.pop(frameData, framePts)) {
    // 解码 frameData...
}
```

- [ ] **Step 4: 编译验证**

成功标准：编译通过，帧率稳定。

---

### Task 8: 视频网络包协议（步骤 13）

**文件：**
- 修改：`Net/def.h`（新增 `STRU_VIDEO_PACKET`）

- [ ] **Step 1: 在 def.h 添加协议定义**

```cpp
#define _DEF_PACK_VIDEO (1028)

struct STRU_VIDEO_PACKET {
    int type;    // _DEF_PACK_VIDEO
    int id;      // userId
    int width;
    int height;
    int pts;
    int len;
    char buf[];  // 柔性数组，H.264 编码数据

    void init() {
        type = _DEF_PACK_VIDEO;
        id = 1;
        width = 320;
        height = 240;
        pts = 0;
        len = 0;
    }
};
```

- [ ] **Step 2: 修改 video_read.cpp 发送端封包**

在 `sig_videoFrame` 发射处，用 `STRU_VIDEO_PACKET` 包裹 H.264 数据：
```cpp
int totalLen = sizeof(STRU_VIDEO_PACKET) + encodedLen;
STRU_VIDEO_PACKET* pkt = (STRU_VIDEO_PACKET*)malloc(totalLen);
pkt->init();
pkt->id = m_userId;
pkt->width = IMAGE_WIDTH;
pkt->height = IMAGE_HEIGHT;
pkt->pts = pts;
pkt->len = encodedLen;
memcpy(pkt->buf, encodedData, encodedLen);
// 通过信号发送
Q_EMIT sig_videoPacket((const char*)pkt, totalLen);
free(pkt);
```

- [ ] **Step 3: 编译验证**

成功标准：编译通过，`STRU_VIDEO_PACKET` 大小计算正确。

---

### Task 9: 跨步骤集成与端到端测试

**文件：**
- 修改：`meetingdialog.cpp`（全部条件编译分支连通）
- 无需新建文件

- [ ] **Step 1: 确认所有 feature flag 独立可切换**

| 宏 | 位置 | 默认 |
|----|------|------|
| `USE_SDL2` | audio_common.h | 开启 |
| `USE_OPENGL` | opengl_render.h | 开启 |
| `USE_OPUS` | audio_common.h | 开启 |
| `USE_H264` | h264_encoder.h | 开启 |

- [ ] **Step 2: 完整编译**

```bash
export PATH="/d/SDKs/Qt/5.15.2/mingw81_32/bin:/d/SDKs/Qt/Tools/mingw810_32/bin:$PATH"
cd "D:/Projects/dev-projects/cpp/qt/demo/WeMeetClient"
qmake WeMeetClient.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile.Release clean
mingw32-make -f Makefile.Release
```

- [ ] **Step 3: 端到端冒烟测试**

启动客户端 → 登录 → 创建会议 → 验证：
- 音频采集/播放正常（SDL2 路径）
- 视频采集/显示正常（H.264 编码 + OpenGL 渲染）
- 人脸检测+萌拍正常
- 关闭 `USE_SDL2` 后回退 Qt Audio 正常
- 关闭 `USE_OPENGL` 后回退 QPainter 正常

- [ ] **Step 4: 回归测试**

```bash
python "D:/Projects/dev-projects/cpp/qt/demo/WeMeetClient/test/smoke_test.py" --test-only
```

---

## 依赖顺序

```
Task 1 (SDL 采集) ──→ Task 2 (SDL 播放) ──→ Task 3 (集成到 meetingdialog)
                                                    ↓
Task 4 (PTS 同步) ──────────────────────→ Task 7 (编码优化) ──→ Task 8 (网络包)
                                                    ↓
Task 5 (OpenGL 渲染) ──→ Task 6 (集成 OpenGL) ──────→ Task 9 (端到端测试)
```

- Task 1-3（SDL2 音频）可与 Task 5-6（OpenGL 视频）**并行开发**
- Task 4（PTS）可与 Task 1-3 **并行开发**
- Task 7-8 依赖 Task 4 完成
- Task 9 依赖所有前序任务完成
