#include "tracelog.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>

static HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION g_csLog;

void initTraceLog() {
    InitializeCriticalSection(&g_csLog);
    char path[256];
    snprintf(path, sizeof(path), "C:\\temp\\wemeet_trace_%lu.log", GetCurrentProcessId());
    g_hLogFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (g_hLogFile == INVALID_HANDLE_VALUE) return;

    // BOM for UTF-8
    DWORD written;
    WriteFile(g_hLogFile, "\xEF\xBB\xBF", 3, &written, NULL);

    TRACE("========== 诊断日志启动 ==========");
}

void traceLog(const char* file, int line, const char* func, const char* fmt, ...) {
    if (g_hLogFile == INVALID_HANDLE_VALUE) return;

    // 时间戳
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[1024];
    int offset = snprintf(buf, sizeof(buf),
        "%02d:%02d:%02d.%03d [%05lu] %s:%d %s() ",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        GetCurrentThreadId(), file, line, func);

    // 用户消息
    va_list args;
    va_start(args, fmt);
    offset += vsnprintf(buf + offset, sizeof(buf) - offset - 2, fmt, args);
    va_end(args);

    buf[offset++] = '\r';
    buf[offset++] = '\n';
    buf[offset] = '\0';

    EnterCriticalSection(&g_csLog);
    DWORD written;
    WriteFile(g_hLogFile, buf, offset, &written, NULL);
    FlushFileBuffers(g_hLogFile);
    LeaveCriticalSection(&g_csLog);
}
