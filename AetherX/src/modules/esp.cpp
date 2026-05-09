#include "pch.h"
#include "modules/esp.h"
#include "sdk/minecraft.h"
#include "sdk/world.h"
#include "render/esp_renderer.h"
#include "core/debug_console.h"

ESP::ESP() : Module("ESP", 'E') {
    m_enabled = false;  // ESP off by default
    m_boolSettings["draw_boxes"]  = true;
    m_boolSettings["draw_names"]  = true;
    m_boolSettings["draw_health"] = true;
    m_boolSettings["draw_distance"] = true;
    m_boolSettings["draw_animals"] = true;
    m_boolSettings["draw_items"] = false;
    m_boolSettings["draw_objects"] = false;
    m_boolSettings["draw_chests"] = false;
    m_boolSettings["chams"]       = false;
    m_floatSettings["max_distance"] = 200.0f;

    // Default colors
    m_colorSettings["box_visible"] = IM_COL32(140, 100, 255, 200); // Purple-ish
    m_colorSettings["text_color"]  = IM_COL32(255, 255, 255, 255); // White
}

void ESP::onRender() {
    if (!m_enabled) return;

    JNIEnv* env = JvmWrapper::getEnv();
    if (!env) {
        static int noEnvFrames = 0;
        if (++noEnvFrames % 120 == 1) {
            DebugConsole::log("ESP", "skipped: JNI env is null");
        }
        return;
    }

    env->PushLocalFrame(256);

    jobject localPlayer = CMinecraft::getPlayer();
    if (!localPlayer) {
        static int noPlayerFrames = 0;
        if (++noPlayerFrames % 120 == 1) {
            DebugConsole::log("ESP", "skipped: local player is null");
        }
        env->PopLocalFrame(nullptr);
        return;
    }

    // Get camera data from the render Camera (interpolated position — matches what the player sees)
    auto camData = CMinecraft::getCameraData();

    double camX, camY, camZ;
    float  yaw, pitch;
    
    if (camData.valid) {
        // Use the actual render camera — already interpolated between ticks
        camX  = camData.x;
        camY  = camData.y;
        camZ  = camData.z;
        yaw   = camData.yaw;
        pitch = camData.pitch;
        EspRenderer::setFOV(camData.fov);
        EspRenderer::setMatrices(false, camData.viewMatrix, camData.projMatrix);
    } else {
        // Fallback: use localPlayer entity position (tick position — may jitter)
        CEntity cam(localPlayer);
        camX  = cam.getX();
        camY  = cam.getY() + 1.62;  // eye height
        camZ  = cam.getZ();
        yaw   = cam.getYaw();
        pitch = cam.getPitch();
    }

    // Get screen size — prefer Minecraft's Window dimensions (exact match for projection matrix)
    int screenW, screenH;
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int glW = viewport[2];
    int glH = viewport[3];
    
    if (camData.valid && camData.screenW > 0 && camData.screenH > 0) {
        // Use Minecraft's actual framebuffer dimensions (what its projection matrix uses)
        screenW = camData.screenW;
        screenH = camData.screenH;
    } else {
        // Fallback to GL viewport
        screenW = glW;
        screenH = glH;
    }
    
    // Debug: compare all three sources periodically
    static int vpDbgCount = 0;
    if (++vpDbgCount <= 5 || vpDbgCount % 1800 == 0) {
        printf("[Ghost] ScreenSize: using=%dx%d GL=%dx%d Window=%dx%d\n",
               screenW, screenH, glW, glH,
               camData.screenW, camData.screenH);
    }

    // Update renderer camera
    EspRenderer::updateCamera(camX, camY, camZ, yaw, pitch, screenW, screenH);

    jobject worldObj = CMinecraft::getWorld();
    if (!worldObj) {
        static int noWorldFrames = 0;
        if (++noWorldFrames % 120 == 1) {
            DebugConsole::log("ESP", "skipped: world is null");
        }
        env->PopLocalFrame(nullptr);
        return;
    }

    CEntity localEntity(localPlayer);
    int localId = localEntity.getId();
    auto players = CWorld::getAllEntities(worldObj);



    bool boxes       = m_boolSettings["draw_boxes"];
    bool names       = m_boolSettings["draw_names"];
    bool health      = m_boolSettings["draw_health"];
    bool distance    = m_boolSettings["draw_distance"];
    bool drawAnimals = m_boolSettings["draw_animals"];
    bool drawItems   = m_boolSettings["draw_items"];
    bool drawObjects = m_boolSettings["draw_objects"];
    bool drawChests  = m_boolSettings["draw_chests"]; // Not yet implemented through Entities list
    bool chams       = m_boolSettings["chams"];
    float maxDist    = m_floatSettings["max_distance"];

    int total = 0;
    int skippedSelf = 0;
    int skippedDead = 0;
    int skippedFilter = 0;
    int skippedDistance = 0;
    int drawn = 0;

    for (auto& entity : players) {
        total++;

        // Skip self
        if (entity.getId() == localId) {
            skippedSelf++;
            continue;
        }
        if (!entity.isAlive()) {
            skippedDead++;
            continue;
        }

        bool isPlayer = entity.isPlayer();
        bool isItem   = entity.isItem();
        bool isArmorStand = entity.isArmorStand();
        bool isMinecartChest = entity.isMinecartChest();
        bool isLiving = entity.isLiving() && !isArmorStand; // Armor stands extend LivingEntity, so manually exclude them

        // Categorize entity
        if (!isPlayer) {
            if (isItem) {
                if (!drawItems) {
                    skippedFilter++;
                    continue;
                }
            } else if (isLiving) {
                if (!drawAnimals) {
                    skippedFilter++;
                    continue;
                }
            } else if (isMinecartChest) {
                if (!drawChests) {
                    skippedFilter++;
                    continue;
                }
            } else {
                // Not player, not item, not living, not chest -> "Object" (e.g. Interaction, Item Frame, Minecart, ArmorStand)
                if (!drawObjects) {
                    skippedFilter++;
                    continue;
                }
            }
        }

        double dx = entity.getX() - camX;
        double dy = entity.getY() - camY;
        double dz = entity.getZ() - camZ;
        double distToEntity = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distToEntity > maxDist) {
            skippedDistance++;
            continue;
        }

        if (chams) {
            entity.setGlowingTag(true);
        } else if (entity.hasGlowingTag()) {
            entity.setGlowingTag(false);
        }

        // Change color based on entity type
        ImU32 boxColor = m_colorSettings["box_visible"];
        if (!isPlayer) {
            if (isItem) {
                boxColor = IM_COL32(255, 255, 0, 200); // Yellow for dropped items
            } else if (isLiving) {
                boxColor = IM_COL32(255, 165, 0, 200); // Orange for animals/mobs
            } else if (isMinecartChest) {
                boxColor = IM_COL32(255, 105, 180, 200); // Hot Pink for Chests
            } else {
                boxColor = IM_COL32(100, 200, 255, 200); // Light blue for objects
            }
        }

        EspRenderer::EspColors colors = {
            boxColor,
            IM_COL32(0, 0, 0, 0), // boxHidden (unimplemented)
            m_colorSettings["text_color"]
        };

        // Don't draw health bars for items and objects
        bool drawEntHealth = health && (isPlayer || isLiving);

        EspRenderer::drawEntity(entity, boxes, names, drawEntHealth, distance, colors);
        drawn++;
    }

    static int espLogFrame = 0;
    if (++espLogFrame % 120 == 1) {
        DebugConsole::log("ESP",
            "enabled=1 world=%p entities=%d drawn=%d self=%d dead=%d filtered=%d distance=%d mobs=%s items=%s objects=%s cam=%s fov=%.1f screen=%dx%d projection=trig",
            worldObj, total, drawn, skippedSelf, skippedDead, skippedFilter, skippedDistance,
            drawAnimals ? "ON" : "OFF",
            drawItems ? "ON" : "OFF",
            drawObjects ? "ON" : "OFF",
            camData.valid ? "render" : "player",
            camData.valid ? camData.fov : 70.0f,
            screenW, screenH);
    }

    env->PopLocalFrame(nullptr);
}
