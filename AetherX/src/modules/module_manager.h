#pragma once
#include "pch.h"
#include "modules/module.h"

/*
 * ModuleManager — Registry and dispatch for all modules.
 */
class ModuleManager {
public:
    static void init();
    static void shutdown();

    static void tickAll(JNIEnv* env);
    static void renderAll();
    static void handleKey(int vk);

    static Module* getModule(const std::string& name);

private:
    static inline std::vector<std::unique_ptr<Module>> s_modules;
};
