#include "pch.h"
#include "render/esp_renderer.h"
#include "sdk/minecraft.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * Trigonometry-based World-to-Screen.
 *
 * Minecraft coordinate system:
 *   yaw=0 → south (+Z), yaw=90 → west (-X), yaw=180 → north (-Z), yaw=-90 → east (+X)
 *   pitch>0 → looking down, pitch<0 → looking up
 *
 * Camera forward vector: (-sin(yaw), -sin(pitch)*cos(yaw... no, simpler:
 *   forward = (-sin(yaw)*cos(pitch), -sin(pitch), cos(yaw)*cos(pitch))
 *
 * View-space transform:
 *   1. Subtract camera position
 *   2. Rotate by +yaw around Y to align forward with +Z
 *   3. Rotate by -pitch around X to level the view
 *   4. Perspective divide using FOV
 */

static int s_debugTick = 0;

void EspRenderer::updateCamera(double camX, double camY, double camZ,
                                float yaw, float pitch,
                                int screenW, int screenH) {
    s_camX = camX;
    s_camY = camY;
    s_camZ = camZ;
    s_yaw  = yaw;
    s_pitch = pitch;
    s_screenW = screenW;
    s_screenH = screenH;
}
void EspRenderer::setMatrices(bool hasMatrices, const float* view, const float* proj) {
    s_hasMatrices = false;
    (void)hasMatrices;
    (void)view;
    (void)proj;
}

bool EspRenderer::worldToScreen(double x, double y, double z, ImVec2& out) {
    // Relative position (target minus camera)
    double dx = x - s_camX;
    double dy = y - s_camY;
    double dz = z - s_camZ;

    if (s_hasMatrices) {
        // 4x4 Matrix multiplication (column-major)
        // Vector is (dx, dy, dz, 1.0)
        double clipX = dx * s_viewMatrix[0] + dy * s_viewMatrix[4] + dz * s_viewMatrix[8]  + 1.0 * s_viewMatrix[12];
        double clipY = dx * s_viewMatrix[1] + dy * s_viewMatrix[5] + dz * s_viewMatrix[9]  + 1.0 * s_viewMatrix[13];
        double clipZ = dx * s_viewMatrix[2] + dy * s_viewMatrix[6] + dz * s_viewMatrix[10] + 1.0 * s_viewMatrix[14];
        double clipW = dx * s_viewMatrix[3] + dy * s_viewMatrix[7] + dz * s_viewMatrix[11] + 1.0 * s_viewMatrix[15];

        // Now multiply by projection matrix
        double ndcX = clipX * s_projMatrix[0] + clipY * s_projMatrix[4] + clipZ * s_projMatrix[8]  + clipW * s_projMatrix[12];
        double ndcY = clipX * s_projMatrix[1] + clipY * s_projMatrix[5] + clipZ * s_projMatrix[9]  + clipW * s_projMatrix[13];
        double ndcZ = clipX * s_projMatrix[2] + clipY * s_projMatrix[6] + clipZ * s_projMatrix[10] + clipW * s_projMatrix[14];
        double ndcW = clipX * s_projMatrix[3] + clipY * s_projMatrix[7] + clipZ * s_projMatrix[11] + clipW * s_projMatrix[15];

        if (ndcW < 0.1f) return false; // Behind camera

        // Perspective divide
        ndcX /= ndcW;
        ndcY /= ndcW;

        // Convert to screen space (Y is inverted)
        out.x = (float)((ndcX * 0.5 + 0.5) * s_screenW);
        out.y = (float)((1.0 - (ndcY * 0.5 + 0.5)) * s_screenH);
        return true;
    }

    // Fallback: Trigonometry
    double yawRad   = s_yaw   * (M_PI / 180.0);
    double pitchRad = s_pitch * (M_PI / 180.0);

    double cosYaw   = cos(yawRad);
    double sinYaw   = sin(yawRad);
    double cosPitch = cos(pitchRad);
    double sinPitch = sin(pitchRad);

    double viewX = -(dx * cosYaw + dz * sinYaw);
    double viewY = dy;
    double viewZ = -dx * sinYaw + dz * cosYaw;

    double finalX = viewX;
    double finalY = viewY * cosPitch + viewZ * sinPitch;
    double finalZ = -viewY * sinPitch + viewZ * cosPitch;

    if (finalZ <= 0.05) return false;

    double fovRad = s_fov * (M_PI / 180.0);
    double halfTanFov = tan(fovRad / 2.0);
    double aspect = (double)s_screenW / (double)s_screenH;
    double fallbackNdcX =  (finalX / (finalZ * halfTanFov * aspect));
    double fallbackNdcY = -(finalY / (finalZ * halfTanFov));

    out.x = (float)((fallbackNdcX * 0.5 + 0.5) * s_screenW);
    out.y = (float)((fallbackNdcY * 0.5 + 0.5) * s_screenH);

    return true;
}

void EspRenderer::drawEntity(const CEntity& entity, bool drawBox, bool drawName, bool drawHealth, bool drawDistance, const EspColors& colors) {
    if (!entity.isValid()) return;

    double ex = entity.getX();
    double ey = entity.getY();
    double ez = entity.getZ();

    auto bbox = entity.getBoundingBox();

    bool bboxValid =
        std::isfinite(bbox.minX) && std::isfinite(bbox.minY) && std::isfinite(bbox.minZ) &&
        std::isfinite(bbox.maxX) && std::isfinite(bbox.maxY) && std::isfinite(bbox.maxZ) &&
        bbox.maxX > bbox.minX && bbox.maxY > bbox.minY && bbox.maxZ > bbox.minZ;

    if (!bboxValid) {
        double halfWidth = entity.isItem() ? 0.20 : 0.35;
        double height = entity.isLiving() ? 1.80 : 0.50;
        bbox.minX = ex - halfWidth;
        bbox.maxX = ex + halfWidth;
        bbox.minY = ey;
        bbox.maxY = ey + height;
        bbox.minZ = ez - halfWidth;
        bbox.maxZ = ez + halfWidth;
    }

    ImVec2 corners[8];
    bool visible[8];
    double pts[8][3] = {
        {bbox.minX, bbox.minY, bbox.minZ},
        {bbox.maxX, bbox.minY, bbox.minZ},
        {bbox.minX, bbox.maxY, bbox.minZ},
        {bbox.maxX, bbox.maxY, bbox.minZ},
        {bbox.minX, bbox.minY, bbox.maxZ},
        {bbox.maxX, bbox.minY, bbox.maxZ},
        {bbox.minX, bbox.maxY, bbox.maxZ},
        {bbox.maxX, bbox.maxY, bbox.maxZ},
    };

    bool anyVisible = false;
    int visCount = 0;
    for (int i = 0; i < 8; i++) {
        visible[i] = worldToScreen(pts[i][0], pts[i][1], pts[i][2], corners[i]);
        if (visible[i]) { anyVisible = true; visCount++; }
    }



    ImVec2 topCenter{}, bottomCenter{};
    bool topVisible = worldToScreen((bbox.minX + bbox.maxX) * 0.5, bbox.maxY, (bbox.minZ + bbox.maxZ) * 0.5, topCenter);
    bool bottomVisible = worldToScreen((bbox.minX + bbox.maxX) * 0.5, bbox.minY, (bbox.minZ + bbox.maxZ) * 0.5, bottomCenter);

    if (!anyVisible && !(topVisible && bottomVisible)) return;

    float minSX = 99999.f, minSY = 99999.f;
    float maxSX = -99999.f, maxSY = -99999.f;

    if (topVisible && bottomVisible) {
        float boxHeight = std::max(12.0f, std::abs(bottomCenter.y - topCenter.y));
        float boxWidth = std::max(8.0f, boxHeight * 0.45f);
        float centerX = (topCenter.x + bottomCenter.x) * 0.5f;

        minSX = centerX - boxWidth * 0.5f;
        maxSX = centerX + boxWidth * 0.5f;
        minSY = std::min(topCenter.y, bottomCenter.y);
        maxSY = std::max(topCenter.y, bottomCenter.y);
    }

    if (visCount >= 4) {
        for (int i = 0; i < 8; i++) {
            if (!visible[i]) continue;
            minSX = std::min(minSX, corners[i].x);
            minSY = std::min(minSY, corners[i].y);
            maxSX = std::max(maxSX, corners[i].x);
            maxSY = std::max(maxSY, corners[i].y);
        }
    }

    if (minSX > maxSX || minSY > maxSY) return;
    if (maxSX < 0.0f || maxSY < 0.0f || minSX > (float)s_screenW || minSY > (float)s_screenH) return;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    if (drawBox) {
        // Outer black outline
        draw->AddRect(ImVec2(minSX - 1, minSY - 1), ImVec2(maxSX + 1, maxSY + 1), IM_COL32(0, 0, 0, 120), 0.0f, 0, 1.0f);
        // Inner colored box
        draw->AddRect(ImVec2(minSX, minSY), ImVec2(maxSX, maxSY), colors.boxVisible, 0.0f, 0, 1.5f);
    }

    if (drawName) {
        std::string name = entity.getName();
        if (name.empty() || name == "Unknown") {
            name = entity.isPlayer() ? "Player" : (entity.isLiving() ? "Mob" : "Entity");
        }
        ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
        float textX = std::clamp((minSX + maxSX) / 2.0f - textSize.x / 2.0f, 2.0f, std::max(2.0f, (float)s_screenW - textSize.x - 2.0f));
        float textY = std::clamp(minSY - textSize.y - 4.0f, 2.0f, std::max(2.0f, (float)s_screenH - textSize.y - 2.0f));

        draw->AddRectFilled(ImVec2(textX - 2, textY - 1), ImVec2(textX + textSize.x + 2, textY + textSize.y + 1), IM_COL32(0, 0, 0, 150), 2.0f);
        draw->AddText(ImVec2(textX, textY), colors.text, name.c_str());
    }

    if (drawDistance) {
        double dx = ex - s_camX;
        double dy = ey - s_camY;
        double dz = ez - s_camZ;
        double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        char distStr[32];
        snprintf(distStr, sizeof(distStr), "[%.1fm]", dist);
        
        ImVec2 textSize = ImGui::CalcTextSize(distStr);
        float textX = std::clamp((minSX + maxSX) / 2.0f - textSize.x / 2.0f, 2.0f, std::max(2.0f, (float)s_screenW - textSize.x - 2.0f));
        float textY = std::clamp(maxSY + 2.0f, 2.0f, std::max(2.0f, (float)s_screenH - textSize.y - 2.0f));

        draw->AddRectFilled(ImVec2(textX - 2, textY - 1), ImVec2(textX + textSize.x + 2, textY + textSize.y + 1), IM_COL32(0, 0, 0, 150), 2.0f);
        draw->AddText(ImVec2(textX, textY), colors.text, distStr);
    }

    if (drawHealth) {
        float health = entity.getHealth();
        float maxHP  = entity.getMaxHealth();
        if (maxHP > 0.0f) {
            float ratio = std::max(0.0f, std::min(1.0f, health / maxHP));
            float barWidth = 3.0f;
            float barX = maxSX + 3.0f;
            float barHeight = maxSY - minSY;

            draw->AddRectFilled(ImVec2(barX, minSY), ImVec2(barX + barWidth, maxSY), IM_COL32(0, 0, 0, 150));
            ImU32 color = (ratio > 0.5f) ? 
                IM_COL32((int)((1.0f - ratio) * 2 * 255), 255, 0, 255) : 
                IM_COL32(255, (int)(ratio * 2 * 255), 0, 255);

            draw->AddRectFilled(ImVec2(barX, minSY + barHeight * (1.0f - ratio)), ImVec2(barX + barWidth, maxSY), color);
        }
    }
}

void EspRenderer::draw3DBox(double minX, double minY, double minZ, double maxX, double maxY, double maxZ, ImU32 color, float thickness) {
    ImVec2 corners[8];
    bool visible[8];
    double pts[8][3] = {
        {minX, minY, minZ}, // 0
        {maxX, minY, minZ}, // 1
        {minX, maxY, minZ}, // 2
        {maxX, maxY, minZ}, // 3
        {minX, minY, maxZ}, // 4
        {maxX, minY, maxZ}, // 5
        {minX, maxY, maxZ}, // 6
        {maxX, maxY, maxZ}, // 7
    };

    bool anyVisible = false;
    for (int i = 0; i < 8; i++) {
        visible[i] = worldToScreen(pts[i][0], pts[i][1], pts[i][2], corners[i]);
        if (visible[i]) anyVisible = true;
    }

    if (!anyVisible) return;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    auto drawLine = [&](int a, int b) {
        if (visible[a] && visible[b]) {
            draw->AddLine(corners[a], corners[b], color, thickness);
        }
    };

    // Bottom face
    drawLine(0, 1);
    drawLine(1, 5);
    drawLine(5, 4);
    drawLine(4, 0);

    // Top face
    drawLine(2, 3);
    drawLine(3, 7);
    drawLine(7, 6);
    drawLine(6, 2);

    // Pillars
    drawLine(0, 2);
    drawLine(1, 3);
    drawLine(4, 6);
    drawLine(5, 7);
}
