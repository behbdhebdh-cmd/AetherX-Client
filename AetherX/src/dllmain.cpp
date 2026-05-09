#include "pch.h"
#include "core/hooks.h"
#include "core/jvm_wrapper.h"
#include "core/debug_console.h"
#include "modules/module_manager.h"

static HMODULE g_hModule = nullptr;
static std::atomic<bool> g_running = true;

static DWORD WINAPI InitThread(LPVOID param) {
    DebugConsole::init();

    // ── Init JNI ────────────────────────────────────────────────────
    if (!JvmWrapper::init()) {
        Sleep(3000);
        DebugConsole::shutdown();
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }

    // ── Init Module System (caches all JNI IDs) ─────────────────────
    ModuleManager::init();

    // ── Init Hooks (wglSwapBuffers) ─────────────────────────────────
    if (!Hooks::init()) {
        JvmWrapper::shutdown();
        Sleep(3000);
        DebugConsole::shutdown();
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }

    printf("\n[Ghost] Initialization complete!\n\n");

    // ── Main loop: wait for unload key ──────────────────────────────
    while (g_running) {
        if (GetAsyncKeyState(VK_DELETE) & 1) {
            g_running = false;
            break;
        }
        Sleep(100);
    }

    // ── Cleanup ─────────────────────────────────────────────────────
    ModuleManager::shutdown();
    Hooks::shutdown();
    JvmWrapper::shutdown();
    DebugConsole::shutdown();

    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr));
    }
    return TRUE;
}
