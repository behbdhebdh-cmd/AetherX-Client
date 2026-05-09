#pragma once
#include "pch.h"
#include "sdk/entity.h"

/*
 * EspRenderer — World-to-screen projection and 2D ESP drawing.
 *
 * Uses trigonometry-based W2S from camera yaw/pitch/position/FOV.
 * This avoids calling any RenderSystem Java methods from the hook thread,
 * which would crash with "Exception in thread Render thread".
 */
class EspRenderer {
public:
    // Call once per frame with the local player's camera data
    static void updateCamera(double camX, double camY, double camZ,
                             float yaw, float pitch,
                             int screenW, int screenH);

    // Set FOV (default 70, can be updated from game options)
    static void setFOV(float fov) { s_fov = fov; }

    // Set projection matrix values from RenderSystem (overrides FOV-based calc)
    static void setMatrices(bool hasMatrices, const float* view, const float* proj);

    static int getScreenW() { return s_screenW; }
    static int getScreenH() { return s_screenH; }

    // Project 3D world coordinates to 2D screen coordinates
    // Returns false if the point is behind the camera
    static bool worldToScreen(double x, double y, double z, ImVec2& out);

    // Draw ESP elements for an entity
    struct EspColors {
        ImU32 boxVisible;
        ImU32 boxHidden;
        ImU32 text;
    };
    static void drawEntity(const CEntity& entity, bool drawBox, bool drawName, bool drawHealth, bool drawDistance, const EspColors& colors);

    // Draw a 3D bounding box (e.g. for blocks)
    static void draw3DBox(double minX, double minY, double minZ, double maxX, double maxY, double maxZ, ImU32 color, float thickness = 1.0f);

private:
    // Camera state (updated per frame)
    static inline double s_camX = 0, s_camY = 0, s_camZ = 0;
    static inline float  s_yaw  = 0, s_pitch = 0;
    static inline int    s_screenW = 1920, s_screenH = 1080;
    static inline float  s_fov  = 70.0f;
    static inline bool   s_hasMatrices = false;
    static inline float  s_viewMatrix[16] = {0};
    static inline float  s_projMatrix[16] = {0};
};
