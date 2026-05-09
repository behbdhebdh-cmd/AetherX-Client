#pragma once
#include "pch.h"

/*
 * Hooks — MinHook-based wglSwapBuffers + WNDPROC hooking.
 */
class Hooks {
public:
    static bool init();
    static void shutdown();

private:
    // wglSwapBuffers hook
    using wglSwapBuffers_t = BOOL(WINAPI*)(HDC);
    static BOOL WINAPI hkWglSwapBuffers(HDC hdc);
    static inline wglSwapBuffers_t oWglSwapBuffers = nullptr;

    // WNDPROC hook
    static LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static inline WNDPROC oWndProc = nullptr;

    static inline bool s_initialized = false;
    static inline HWND  s_hwnd       = nullptr;
};
