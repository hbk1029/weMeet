# WeMeet - 视频会议系统

基于 C++ Qt5 框架开发的视频会议系统，支持实时音视频通话、文字聊天、好友管理。

## 项目结构

```
WeMeetClient/     # Qt5 客户端（Windows）
WeMeetServer/     # C++ 服务端（Linux，需 MySQL + BCrypt）
```

## 功能特性

- 用户注册/登录（BCrypt 密码加密）
- 好友系统（添加/删除/搜索/在线状态推送）
- 好友请求管理（发送/接收/同意/拒绝）
- 会议创建/加入/退出/结束
- 会议文字聊天
- Speex 8kHz 窄带音频实时通话
- OpenCV 视频采集与显示（320×240 JPEG）
- 麦克风静音 / 摄像头开关
- 会议历史记录

## 技术栈

| 组件 | 技术 |
|------|------|
| 客户端 UI | Qt 5.15.2 (Widgets + Designer) |
| 客户端编译 | MinGW 8.1 64bit |
| 服务端 | Ubuntu 22.04 + GCC |
| 网络 | TCP 长连接，自定义二进制协议 |
| 数据库 | MySQL (libmysqlclient) |
| 密码加密 | BCrypt |
| 音频 | Speex 8kHz NB Codec |
| 视频 | OpenCV 4.2.0 |

## 快速开始

### 服务端

```bash
cd WeMeetServer
make clean && make
./WeMeetServer
```

需要 MySQL 数据库 `wechat_db`，包含 `t_user`, `t_friend`, `t_friend_requests`, `t_meeting`, `t_meeting_member` 表。

### 客户端

1. 安装 Qt 5.15.2 MinGW 64bit
2. 配置 OpenCV 和 Speex 库路径
3. 修改 `Net/def.h` 中的 `_DEF_SERVER_IP` 为服务端地址
4. 编译运行：

```bash
qmake WeMeetClient.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile.Release
./release/WeMeetClient.exe
```

## 协议

自定义二进制协议，基于 `_DEF_PROTOCOL_BASE=1000`，当前共 40 个协议类型。
关键协议类型定义在 `Net/def.h`（客户端和服务端需保持同步）。

## 测试

```bash
cd test
python smoke_test.py --build   # 完整重建服务端+测试
python smoke_test.py            # 冒烟测试
python smoke_test.py --test-only  # 仅协议测试
```

## 当前状态（2026-05-19）

MVP 核心功能已就绪：
- 登录/注册、好友管理、会议基础功能
- Speex 音频 + OpenCV 视频
- 好友请求系统（存储到数据库，支持离线请求）

待完善：
- 会议历史持久化（当前内存存储）
- 视频编码优化
- 屏幕共享

## License

MIT
