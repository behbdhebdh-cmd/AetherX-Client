#include "pch.h"
#include "render/menu.h"
#include "modules/module_manager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
    static GLuint s_iconTexture = 0;
    static int s_iconWidth = 0;
    static int s_iconHeight = 0;

    bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
        int image_width = 0;
        int image_height = 0;
        int channels = 0;
        unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &channels, 4);
        if (image_data == NULL) return false;

        // Save previous OpenGL state
        GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        GLint last_unpack_row_length; glGetIntegerv(0x0CF2 /*GL_UNPACK_ROW_LENGTH*/, &last_unpack_row_length);
        GLint last_unpack_alignment; glGetIntegerv(0x0CF5 /*GL_UNPACK_ALIGNMENT*/, &last_unpack_alignment);

        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // GL_CLAMP_TO_EDGE is 0x812F
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); 

        glPixelStorei(0x0CF2 /*GL_UNPACK_ROW_LENGTH*/, 0);
        glPixelStorei(0x0CF5 /*GL_UNPACK_ALIGNMENT*/, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);

        // Restore state
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glPixelStorei(0x0CF2 /*GL_UNPACK_ROW_LENGTH*/, last_unpack_row_length);
        glPixelStorei(0x0CF5 /*GL_UNPACK_ALIGNMENT*/, last_unpack_alignment);

        *out_texture = image_texture;
        *out_width = image_width;
        *out_height = image_height;

        return true;
    }


    void ModuleCheckbox(Module* module, const char* label) {
        if (!module) return;

        bool enabled = module->isEnabled();
        if (ImGui::Checkbox(label, &enabled)) {
            module->setEnabled(enabled);
        }
    }

    void SettingCheckbox(Module* module, const char* key, const char* label) {
        if (!module) return;
        ImGui::Checkbox(label, &module->getBoolSetting(key));
    }

    void SettingSlider(Module* module, const char* key, const char* label,
                       float min, float max, const char* format) {
        if (!module) return;
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat(label, &module->getFloatSetting(key), min, max, format);
    }

    void ColorEditor(Module* module, const char* key, const char* label) {
        if (!module) return;

        ImU32& color = module->getColorSetting(key);
        float col[4] = {
            (float)((color >> 0) & 0xFF) / 255.0f,
            (float)((color >> 8) & 0xFF) / 255.0f,
            (float)((color >> 16) & 0xFF) / 255.0f,
            (float)((color >> 24) & 0xFF) / 255.0f
        };

        if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_AlphaBar)) {
            color = IM_COL32((int)(col[0] * 255.0f),
                             (int)(col[1] * 255.0f),
                             (int)(col[2] * 255.0f),
                             (int)(col[3] * 255.0f));
        }
    }

    void SectionTitle(const char* label) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.0f, 1.0f), "%s", label);
        ImGui::Separator();
        ImGui::Spacing();
    }
}

void Menu::TabItem(const char* label, int id) {
    bool selected = (s_selectedTab == id);
    ImGui::PushStyleColor(ImGuiCol_Button,
        selected ? ImVec4(0.12f, 0.36f, 0.58f, 1.0f) : ImVec4(0.13f, 0.15f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.42f, 0.66f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.30f, 0.50f, 1.0f));

    if (ImGui::Button(label, ImVec2(150.0f, 46.0f))) {
        s_selectedTab = id;
    }

    ImGui::PopStyleColor(3);
}

void Menu::CustomToggle(const char* label, bool* v) {
    ImGui::Checkbox(label, v);
}

void Menu::CustomSlider(const char* label, float* v, float min, float max, const char* format) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat(label, v, min, max, format);
}

void Menu::applyTheme() {
    s_boldFont = nullptr;

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    style.ScaleAllSizes(1.55f);

    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.WindowBorderSize = 2.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(22.0f, 18.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(12.0f, 11.0f);
    style.ItemInnerSpacing = ImVec2(10.0f, 8.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.96f, 0.98f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.58f, 0.63f, 0.70f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.030f, 0.038f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.065f, 0.078f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.05f, 0.06f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.34f, 0.43f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.13f, 0.32f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.025f, 0.030f, 0.038f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.025f, 0.030f, 0.038f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.38f, 0.78f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.38f, 0.78f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.72f, 0.90f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.42f, 0.66f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.36f, 0.58f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.42f, 0.66f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.10f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.30f, 0.36f, 1.00f);
}

void Menu::render() {
    if (!s_visible) return;

    static bool themeApplied = false;
    if (!themeApplied) {
        applyTheme();
        // LoadTextureFromFile("C:\\Users\\benar\\Downloads\\cheat\\Null Client\\Null Client\\Null Client\\ClientIcon.png", &s_iconTexture, &s_iconWidth, &s_iconHeight);
        themeApplied = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* background = ImGui::GetBackgroundDrawList();
    background->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 120));

    const float winW = std::min(720.0f, std::max(560.0f, io.DisplaySize.x - 64.0f));
    const float winH = std::min(540.0f, std::max(430.0f, io.DisplaySize.y - 64.0f));
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - winW) * 0.5f,
                                   (io.DisplaySize.y - winH) * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGui::Begin("AetherX // Forge 1.21.1", &s_visible,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    /*
    if (s_iconTexture != 0) {
        ImGui::Image((void*)(intptr_t)s_iconTexture, ImVec2(32, 32));
        ImGui::SameLine();
    }
    */
    
    ImGui::TextColored(ImVec4(0.90f, 0.96f, 1.00f, 1.0f), "AetherX");
    ImGui::SameLine();
    ImGui::TextDisabled("Forge 1.21.1");
    ImGui::Separator();

    TabItem("COMBAT", 0);
    ImGui::SameLine();
    TabItem("MOVEMENT", 1);
    ImGui::SameLine();
    TabItem("VISUALS", 2);
    ImGui::SameLine();
    TabItem("UTILITY", 3);

    ImGui::Spacing();
    ImGui::BeginChild("MainContent", ImVec2(0, -34.0f), true);

    if (s_selectedTab == 0) {
        SectionTitle("COMBAT MODULES");

        Module* reach = ModuleManager::getModule("Reach");
        ModuleCheckbox(reach, "Enable Reach");
        if (reach && reach->isEnabled()) {
            SettingSlider(reach, "reach_distance", "Reach Distance", 3.0f, 10.0f, "%.1f blocks");
        }

        ImGui::Spacing();
        Module* aim = ModuleManager::getModule("Aim Assist");
        ModuleCheckbox(aim, "Enable Aim Assist");
        if (aim && aim->isEnabled()) {
            SettingCheckbox(aim, "aim_assist", "Interpolation");
            SettingSlider(aim, "aim_fov", "FOV Radius", 10.0f, 180.0f, "%.0f deg");
            SettingSlider(aim, "aim_speed", "Smoothness", 0.1f, 5.0f, "%.1f");
            SettingCheckbox(aim, "triggerbot", "Triggerbot");
            SettingCheckbox(aim, "auto_clicker", "AutoClicker");
            SettingSlider(aim, "cps", "CPS Max", 1.0f, 20.0f, "%.0f");
        }
    } else if (s_selectedTab == 1) {
        SectionTitle("MOVEMENT MODULES");

        Module* jesus = ModuleManager::getModule("Jesus");
        ModuleCheckbox(jesus, "Enable Jesus");
        if (jesus && jesus->isEnabled()) {
            SettingCheckbox(jesus, "solid", "Solid Collision");
            SettingSlider(jesus, "bounce", "Surface Bounce", 0.0f, 0.4f, "%.2f");
        }
    } else if (s_selectedTab == 2) {
        SectionTitle("VISUAL MODULES");

        Module* esp = ModuleManager::getModule("ESP");
        ModuleCheckbox(esp, "Enable ESP");
        if (esp && esp->isEnabled()) {
            SettingCheckbox(esp, "draw_boxes", "2D Bounding Boxes");
            SettingCheckbox(esp, "draw_names", "Names");
            SettingCheckbox(esp, "draw_health", "Health Bars");
            SettingCheckbox(esp, "draw_distance", "Distance");
            SettingCheckbox(esp, "draw_animals", "Mobs / Animals");
            SettingCheckbox(esp, "draw_items", "Dropped Items");
            SettingCheckbox(esp, "draw_objects", "Other Objects");
            SettingCheckbox(esp, "chams", "Glowing Chams");
            SettingSlider(esp, "max_distance", "Visual Range", 10.0f, 500.0f, "%.0fm");

            ImGui::Spacing();
            ColorEditor(esp, "box_visible", "Box Color");
            ColorEditor(esp, "text_color", "Text Color");
        }
    } else {
        SectionTitle("UTILITY MODULES");

        Module* xray = ModuleManager::getModule("X-Ray");
        ModuleCheckbox(xray, "Enable X-Ray");
        if (xray && xray->isEnabled()) {
            SettingCheckbox(xray, "diamond", "Diamond");
            SettingCheckbox(xray, "iron", "Iron");
            SettingCheckbox(xray, "gold", "Gold");
            SettingCheckbox(xray, "emerald", "Emerald");
            SettingCheckbox(xray, "coal", "Coal");
            SettingCheckbox(xray, "lapis", "Lapis");
            SettingCheckbox(xray, "redstone", "Redstone");
            SettingCheckbox(xray, "spawner", "Spawners");
            SettingSlider(xray, "radius", "Search Radius", 10.0f, 100.0f, "%.0f");
            SettingSlider(xray, "thickness", "Tracer Thickness", 1.0f, 5.0f, "%.1f");
        }
    }

    ImGui::EndChild();
    ImGui::TextDisabled("INSERT toggles menu | DELETE unloads");
    ImGui::End();
}
