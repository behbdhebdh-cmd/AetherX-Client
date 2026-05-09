#pragma once
#include "pch.h"

class DebugConsole {
public:
    static void init();
    static void shutdown();
    static void log(const char* category, const char* fmt, ...);

private:
    static inline bool s_initialized = false;
    static inline FILE* s_logFile = nullptr;
    static inline std::mutex s_mutex;
};
