#include "pch.h"
#include "core/hooks.h"
#include "render/renderer.h"
#include "render/menu.h"
#include "render/esp_renderer.h"
#include "modules/module_manager.h"
#include "core/jvm_wrapper.h"

// Forward declare ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

bool Hooks::init() {
    if (MH_Initialize() != MH_OK) {
        printf("[Ghost] ERROR: MinHook init failed\n");
        return false;
    }

    // ── Hook wglSwapBuffers ─────────────────────────────────────────
    HMODULE opengl = GetModuleHandleA("opengl32.dll");
    if (!opengl) {
        printf("[Ghost] ERROR: opengl32.dll not found\n");
        return false;
    }

    void* pSwapBuffers = GetProcAddress(opengl, "wglSwapBuffers");
    if (!pSwapBuffers) {
        printf("[Ghost] ERROR: wglSwapBuffers not found\n");
        return false;
    }

    if (MH_CreateHook(pSwapBuffers, &hkWglSwapBuffers,
                       reinterpret_cast<void**>(&oWglSwapBuffers)) != MH_OK) {
        printf("[Ghost] ERROR: Failed to create wglSwapBuffers hook\n");
        return false;
    }

    if (MH_EnableHook(pSwapBuffers) != MH_OK) {
        printf("[Ghost] ERROR: Failed to enable wglSwapBuffers hook\n");
        return false;
    }

    printf("[Ghost] wglSwapBuffers hooked successfully\n");
    return true;
}

void Hooks::shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    // Restore WNDPROC
    if (s_hwnd && oWndProc) {
        SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
        oWndProc = nullptr;
    }

    Renderer::shutdown();
    printf("[Ghost] Hooks cleaned up\n");
}

BOOL WINAPI Hooks::hkWglSwapBuffers(HDC hdc) {
    // ── One-time init ───────────────────────────────────────────────
    if (!s_initialized) {
        s_hwnd = WindowFromDC(hdc);
        if (s_hwnd) {
            // Hook WNDPROC for input
            oWndProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc))
            );

            Renderer::init(s_hwnd);
            s_initialized = true;
            printf("[Ghost] Renderer initialized\n");
        }
    }

    if (!s_initialized) {
        return oWglSwapBuffers(hdc);
    }

    // ── Update modules (JNI calls) ──────────────────────────────────
    JNIEnv* env = JvmWrapper::getEnv();
    if (env) {
        // Essential: Clear any pending exceptions before we do anything
        if (env->ExceptionCheck()) env->ExceptionClear();

        env->PushLocalFrame(64);
        ModuleManager::tickAll(env);
        env->PopLocalFrame(nullptr);
    }

    // ── Render ImGui ────────────────────────────────────────────────
    Renderer::beginFrame();

    // Menu (toggled with INSERT)
    Menu::render();

    // ESP overlay (renders behind menu if both active)
    ModuleManager::renderAll();

    Renderer::endFrame();

    return oWglSwapBuffers(hdc);
}

LRESULT CALLBACK Hooks::hkWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // Let ImGui handle input first
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) {
        return TRUE;
    }

    // Toggle menu with INSERT
    if (msg == WM_KEYDOWN && wp == VK_INSERT) {
        Menu::toggle();
        return TRUE;
    }

    // Handle module keybinds
    if (msg == WM_KEYDOWN) {
        ModuleManager::handleKey(static_cast<int>(wp));
    }

    // If menu is open, block game input for mouse events
    if (Menu::isVisible()) {
        switch (msg) {
            case WM_LBUTTONDOWN: case WM_LBUTTONUP:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_MOUSEMOVE:
                return TRUE;
        }
    }

    return CallWindowProcA(oWndProc, hwnd, msg, wp, lp);
}
