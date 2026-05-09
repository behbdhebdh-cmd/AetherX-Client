#pragma once
#include "pch.h"
#include "sdk/entity.h"

/*
 * CMinecraft — Singleton accessor for the Minecraft instance.
 */
class CMinecraft {
public:
    static bool initIDs();

    // Get the Minecraft singleton (local ref — call within PushLocalFrame)
    static jobject getInstance();

    // Get current player (local ref)
    static jobject getPlayer();

    // Get current world (local ref)
    static jobject getWorld();

    // ── Camera data (interpolated render position) ──────────────────
    struct CameraData {
        double x, y, z;
        float  yaw, pitch;
        float  fov;
        int    screenW, screenH;  // Minecraft's actual framebuffer dimensions
        bool   hasMatrices;
        float  viewMatrix[16];    // 4x4 column-major ModelView Matrix
        float  projMatrix[16];    // 4x4 column-major Projection Matrix
        bool   valid;
    };
    // Returns the render camera's interpolated position/rotation + FOV
    static CameraData getCameraData();

private:
    static inline jclass    s_mcClass        = nullptr;
    static inline jmethodID s_getInstance    = nullptr;
    static inline jfieldID  s_instanceField  = nullptr;  // static 'instance' field
    static inline jfieldID  s_player         = nullptr;
    static inline jfieldID  s_level          = nullptr;
    static inline jfieldID  s_gameRenderer   = nullptr;
    static inline jfieldID  s_options        = nullptr;

    // GameRenderer
    static inline jclass    s_grClass        = nullptr;
    static inline jmethodID s_getMainCamera    = nullptr;
    static inline jmethodID s_getGRProjMatrix  = nullptr;
    static inline jmethodID s_getFov           = nullptr;

    // Camera (uses FIELDS, not methods — Camera is not an Entity subclass)
    static inline jclass    s_camClass       = nullptr;
    static inline jfieldID  s_camPosition    = nullptr;  // Vec3 position
    static inline jfieldID  s_camYRot        = nullptr;  // float yRot
    static inline jfieldID  s_camXRot        = nullptr;  // float xRot

    // Vec3 fields (cached from CEntity — reuse)
    // Options FOV
    static inline jclass    s_optionsClass   = nullptr;
    static inline jfieldID  s_optFov         = nullptr;  // OptionInstance<Integer>
    static inline jmethodID s_optInstGet     = nullptr;  // OptionInstance.get()

    // Window (Minecraft.getWindow() → framebuffer dimensions)
    static inline jfieldID  s_window         = nullptr;  // Minecraft.window field
    static inline jclass    s_windowClass    = nullptr;
    static inline jmethodID s_getWidth       = nullptr;  // Window.getWidth()
    static inline jmethodID s_getHeight      = nullptr;  // Window.getHeight()

    // RenderSystem and Matrix4f
    static inline jclass    s_rsClass        = nullptr;
    static inline jmethodID s_rsGetProj      = nullptr;
    static inline jmethodID s_rsGetModelView = nullptr;
    static inline jclass    s_mat4Class      = nullptr;
    
    // Matrix4f fields (column-major)
    static inline jfieldID  s_mat4_m00 = nullptr, s_mat4_m01 = nullptr, s_mat4_m02 = nullptr, s_mat4_m03 = nullptr;
    static inline jfieldID  s_mat4_m10 = nullptr, s_mat4_m11 = nullptr, s_mat4_m12 = nullptr, s_mat4_m13 = nullptr;
    static inline jfieldID  s_mat4_m20 = nullptr, s_mat4_m21 = nullptr, s_mat4_m22 = nullptr, s_mat4_m23 = nullptr;
    static inline jfieldID  s_mat4_m30 = nullptr, s_mat4_m31 = nullptr, s_mat4_m32 = nullptr, s_mat4_m33 = nullptr;
};
