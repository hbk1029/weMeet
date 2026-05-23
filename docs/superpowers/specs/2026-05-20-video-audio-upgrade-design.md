# 视频会议系统 — 音视频引擎升级设计文档

> 日期：2026-05-20 | 基于 `D:\Projects\VideoProject\Video-Better\` 参考路线

## 当前状态

| 模块 | MVP（现有） | 目标 |
|------|------------|------|
| 视频采集编码 | OpenCV 4.2.0 + JPEG q40 / H.264 libx264 | FFmpeg H.264 管线 + 人脸萌拍 |
| 视频显示 | QLabel 贴图 | OpenGL GPU 渲染 |
| 音频采集 | Qt QAudioInput | SDL2 音频采集 |
| 音频播放 | Qt QAudioOutput | SDL2 音频播放 |
| 音频编码 | Speex 8kHz NB / Opus 48kHz | Opus 48kHz 宽带 |
| 人脸检测 | 无 | Haar Cascade + 萌拍贴纸 |
| 网络传输 | 自定义二进制协议 | H.264 AVPacket + Opus 编码流 |
| 时间同步 | 无 | PTS 帧率控制 |

## 执行路线（4 阶段，13 步）

### 阶段一：视频引擎升级

**步骤 01 — 人脸检测强化**
- 新建 `MyFaceDetect` 类，不侵入现有管线
- OpenCV 4.5.4 + Haar Cascade 人脸+眼睛双重检测
- 萌拍贴纸：QPainter 叠加兔耳/圣诞帽到视频帧
- 接入 `video_read.cpp` 采集循环

**步骤 03 — FFmpeg 视频播放器**
- 集成现有 `ffmpeg_decoder.h/cpp` + `ffmpeg_player.h/cpp`
- 解码线程：`av_read_frame` → `avcodec_decode_video2` → `sws_scale` → QImage

**步骤 04 — 视频时间同步**
- PTS 控制帧率，`av_gettime()` 与实际时间比较

**步骤 09 — OpenGL 加速渲染**
- `QOpenGLWidget` 替代 QLabel，GPU YUV→RGB 转换
- 参考：`09-视频加速渲染 (openGL)/opengl/`

### 阶段二：音频引擎升级

**步骤 14 — Opus 编译**
- Opus 1.4（源码 `14-编译opus/opus-1.4/`）
- 参数：`opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP)`

**步骤 05 — SDL2 音频播放**
- SDL2 2.0.10，`SDL_OpenAudioDevice` + 回调队列播放

**步骤 06 — SDL2 音频采集**
- `SDL_OpenAudioDevice(NULL, 1, ...)` 录音模式

**步骤 11 — 音频采集与编码**
- SDL2 采集 → Opus 编码，替换 Qt QAudioInput 路径

### 阶段三：编码与传输

**步骤 10 — 编码项目创建**
- FFmpeg 4.3.1 H.264 编码管线：RGB→YUV420P→AVPacket
- 与现有 `h264_encoder.h/cpp` 整合

**步骤 12 — 编码优化**
- 丢帧策略 + `BufferDataNode` 缓冲队列 + 音视频交错

**步骤 13 — 发送编码 & 接收解码**
- 新协议 `STRU_VIDEO_PACKET`，TCP 收发 H.264 + OpenGL 渲染

### 阶段四：混合与控制

**步骤 07 — 视音频混合**：`av_compare_ts()` 交错写入

**步骤 08 — 播放控制**：暂停/继续/跳转

## 依赖链

```
阶段一(视频)：01 → 03 → 04 → 09
阶段二(音频)：14 → 05 → 06 → 11
阶段三(编码)：10 → 12 → 13
阶段四(混合)：07 → 08

阶段一+二可并行 → 汇合入阶段三 → 阶段四
```

## 依赖库清单

| 库 | 版本 | 路径 |
|----|------|------|
| FFmpeg | 4.3.1 | `10-编码项目创建/ffmpeg-4.3.1/` |
| OpenCV | 4.5.4 + contrib | `01-人脸检测强化/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/` |
| SDL2 | 2.0.10 | `05-音频播放/SDL2-2.0.10/` |
| Opus | 1.4 | `14-编译opus/opus-1.4/` |

## 对现有代码的影响

| 文件 | 改动 |
|------|------|
| `VideoApi/video_read.cpp` | 接入人脸检测+萌拍 |
| `VideoApi/video_write.cpp` | QOpenGLWidget 替换 QLabel |
| `AudioApi/audio_read.cpp` | Qt Audio → SDL2 采集 |
| `AudioApi/audio_write.cpp` | Qt Audio → SDL2 播放 |
| `Net/def.h` | 新增 `STRU_VIDEO_PACKET` |
| 新增 | `myfacedetect.h/cpp`, SDL 封装类, OpenGL 渲染类 |

## Feature Flags

| 宏 | 位置 | 作用 |
|----|------|------|
| `USE_OPUS` | audio_common.h | Opus 48kHz / Speex 8kHz 切换 |
| `USE_H264` | h264_encoder.h | H.264 / JPEG 视频编码切换 |
| `USE_SDL2` | 新增 | SDL2 / Qt Audio 切换 |
| `USE_OPENGL` | 新增 | OpenGL / QLabel 渲染切换 |

## 回退机制

- 每个模块用 feature flag 包裹，可一键切回旧路径
- 新旧代码路径并存于条件分支，稳定后再清理旧代码
- 不删除现有 Qt Audio / QLabel / Speex 路径
