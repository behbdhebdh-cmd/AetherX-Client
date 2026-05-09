#include "pch.h"
#include "modules/module_manager.h"
#include "modules/reach.h"
#include "modules/jesus.h"
#include "modules/esp.h"
#include "modules/xray.h"
#include "modules/aimassist.h"
#include "modules/aimassist.h"
#include "sdk/minecraft.h"
#include "sdk/entity.h"
#include "sdk/world.h"

void ModuleManager::init() {
    // ── Init SDK IDs ────────────────────────────────────────────────
    if (!CMinecraft::initIDs()) {
        printf("[Ghost] WARNING: Minecraft SDK init failed — modules may not work\n");
        printf("[Ghost] TIP: Update class/method names in config/mappings.h\n");
    }

    if (!CEntity::initIDs()) {
        printf("[Ghost] WARNING: Entity SDK init failed\n");
    }

    if (!CWorld::initIDs()) {
        printf("[Ghost] WARNING: World SDK init failed\n");
    }

    // ── Register modules ────────────────────────────────────────────
    auto reg = [&](std::unique_ptr<Module> mod) {
        printf("[Ghost] Registering: %s\n", mod->getName().c_str());
        s_modules.push_back(std::move(mod));
    };

    reg(std::make_unique<Reach>());
    reg(std::make_unique<Jesus>());
    reg(std::make_unique<ESP>());
    reg(std::make_unique<XRay>());
    reg(std::make_unique<AimAssist>());

    printf("[Ghost] %zu modules registered successfully\n", s_modules.size());
}

void ModuleManager::shutdown() {
    for (auto& mod : s_modules) {
        if (mod->isEnabled()) {
            mod->setEnabled(false);
        }
    }
    s_modules.clear();
}

void ModuleManager::tickAll(JNIEnv* env) {
    for (auto& mod : s_modules) {
        if (mod->isEnabled()) {
            mod->onUpdate(env);
        }
    }
}

void ModuleManager::renderAll() {
    static int renderDbg = 0;
    renderDbg++;
    if (renderDbg % 600 == 1) {
        printf("[Ghost] renderAll: %zu modules, enabled: ", s_modules.size());
        for (auto& mod : s_modules) {
            printf("%s=%s ", mod->getName().c_str(), mod->isEnabled() ? "ON" : "OFF");
        }
        printf("\n");
    }
    for (auto& mod : s_modules) {
        if (mod->isEnabled()) {
            mod->onRender();
        }
    }
}

void ModuleManager::handleKey(int vk) {
    for (auto& mod : s_modules) {
        if (mod->getKeyBind() == vk) {
            mod->toggle();
            printf("[Ghost] %s: %s\n", mod->getName().c_str(),
                   mod->isEnabled() ? "ON" : "OFF");
        }
    }
}

Module* ModuleManager::getModule(const std::string& name) {
    for (auto& mod : s_modules) {
        if (mod->getName() == name) {
            return mod.get();
        }
    }
    return nullptr;
}
