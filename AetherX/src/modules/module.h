#pragma once
#include "pch.h"
#include <string>
#include <unordered_map>
#include <variant>

/*
 * Module — Base class for all cheat modules.
 */
class Module {
public:
    Module(const std::string& name, int keyBind)
        : m_name(name), m_keyBind(keyBind) {}

    virtual ~Module() = default;

    // Core lifecycle
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onUpdate(JNIEnv* env) {}
    virtual void onRender() {}  // For overlay drawing (ESP etc.)

    // Toggle
    void toggle() {
        m_enabled = !m_enabled;
        if (m_enabled) onEnable();
        else onDisable();
    }

    void setEnabled(bool v) {
        if (v != m_enabled) {
            m_enabled = v;
            if (m_enabled) onEnable();
            else onDisable();
        }
    }

    bool isEnabled() const { return m_enabled; }
    const std::string& getName() const { return m_name; }
    int getKeyBind() const { return m_keyBind; }

    // ── Settings system ─────────────────────────────────────────────
    float& getFloatSetting(const std::string& key) {
        return m_floatSettings[key];
    }

    bool& getBoolSetting(const std::string& key) {
        return m_boolSettings[key];
    }

    ImU32& getColorSetting(const std::string& key) {
        return m_colorSettings[key];
    }

protected:
    std::string m_name;
    int m_keyBind;
    bool m_enabled = false;

    std::unordered_map<std::string, float> m_floatSettings;
    std::unordered_map<std::string, bool>  m_boolSettings;
    std::unordered_map<std::string, ImU32> m_colorSettings;
};
