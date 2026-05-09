#pragma once
#include "pch.h"

class Menu {
public:
    static void render();
    static void applyTheme();
    static void toggle() { s_visible = !s_visible; }
    static bool isVisible() { return s_visible; }

private:
    // Custom Widgets
    static void CustomToggle(const char* label, bool* v);
    static void CustomSlider(const char* label, float* v, float min, float max, const char* format = "%.1f");
    static void TabItem(const char* label, int id);

    static inline bool s_visible = false;
    static inline int  s_selectedTab = 0;
    static inline ImFont* s_boldFont = nullptr;
};
