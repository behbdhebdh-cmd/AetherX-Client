#pragma once
#include "pch.h"

/*
 * Renderer — ImGui + OpenGL3 bootstrap.
 */
class Renderer {
public:
    static void init(HWND hwnd);
    static void shutdown();
    static void beginFrame();
    static void endFrame();

    static bool isInitialized() { return s_initialized; }

private:
    static void setupStyle();
    static inline bool s_initialized = false;
};
