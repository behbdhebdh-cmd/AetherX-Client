#include "pch.h"
#include "core/debug_console.h"
#include <cstdarg>

void DebugConsole::init() {
    if (s_initialized) return;

    AllocConsole();
    SetConsoleTitleA("Null Client Debug Console");

    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    fopen_s(&s_logFile, "NullClient_debug.log", "a");
    s_initialized = true;
    log("CORE", "Debug console initialized");
}

void DebugConsole::shutdown() {
    if (!s_initialized) return;

    log("CORE", "Debug console shutting down");
    if (s_logFile) {
        fclose(s_logFile);
        s_logFile = nullptr;
    }

    FreeConsole();
    s_initialized = false;
}

void DebugConsole::log(const char* category, const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(s_mutex);

    SYSTEMTIME time{};
    GetLocalTime(&time);

    char msg[1024]{};
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    printf("[%02u:%02u:%02u.%03u][%s] %s\n",
           time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
           category ? category : "LOG", msg);

    if (s_logFile) {
        fprintf(s_logFile, "[%02u:%02u:%02u.%03u][%s] %s\n",
                time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
                category ? category : "LOG", msg);
        fflush(s_logFile);
    }
}
