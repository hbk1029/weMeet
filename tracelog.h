#ifndef TRACELOG_H
#define TRACELOG_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// 线程安全的诊断日志：写入 C:\temp\wemeet_trace_<PID>.log
void initTraceLog();
void traceLog(const char* file, int line, const char* func, const char* fmt, ...);

#define TRACE(fmt, ...) traceLog(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif
