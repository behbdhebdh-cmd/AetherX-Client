#include "pch.h"
#include "modules/aimassist.h"
#include "sdk/minecraft.h"
#include "sdk/entity.h"
#include "sdk/world.h"
#include <cmath>
#include <chrono>

AimAssist::AimAssist() : Module("Aim Assist", 0) {
    m_boolSettings["triggerbot"]   = false;
    m_boolSettings["aim_assist"]   = true;
    m_boolSettings["auto_clicker"] = false;

    m_floatSettings["target_range"] = 4.0f;
    m_floatSettings["aim_fov"]      = 45.0f;
    m_floatSettings["aim_speed"]    = 2.0f; // 0.1 to 10.0 smoothing
    m_floatSettings["cps"]          = 12.0f;
}

float AimAssist::wrapAngleTo180(float angle) {
    angle = std::fmod(angle, 360.0f);
    if (angle >= 180.0f) angle -= 360.0f;
    if (angle < -180.0f) angle += 360.0f;
    return angle;
}

float AimAssist::getAngleDifference(float a, float b) {
    return wrapAngleTo180(a - b);
}

void AimAssist::onUpdate(JNIEnv* env) {
    if (!m_enabled) return;

    jobject mc = CMinecraft::getInstance();
    if (!mc) return;

    jobject playerObj = CMinecraft::getPlayer();
    if (!playerObj) {
        env->DeleteLocalRef(mc);
        return;
    }

    CEntity player(playerObj);

    bool triggerbot  = m_boolSettings["triggerbot"];
    bool aimAssist   = m_boolSettings["aim_assist"];
    bool autoClicker = m_boolSettings["auto_clicker"];
    float targetRange = m_floatSettings["target_range"];
    float aimFov     = m_boolSettings["aim_assist"] ? m_floatSettings["aim_fov"] : 0.0f;
    float aimSpeed   = m_floatSettings["aim_speed"];
    float cps        = m_floatSettings["cps"];

    auto now = std::chrono::steady_clock::now();
    long long nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    bool lmbHeld = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    // ─── 1. Target Selection for Aim Assist ──────────────────────────
    if (aimAssist && lmbHeld) { // Only aim assist while holding attack
        jobject worldObj = CMinecraft::getWorld();
        if (worldObj) {
            std::vector<CEntity> entities = CWorld::getAllEntities(worldObj);
            
            CEntity* bestTarget = nullptr;
            float bestFov = aimFov;

            double px = player.getX();
            double py = player.getY() + 1.62; // approximate eye height
            double pz = player.getZ();
            float pYaw = player.getYaw();
            float pPitch = player.getPitch();

            for (auto& entity : entities) {
                if (entity.getId() == player.getId() || !entity.isAlive() || !entity.isLiving()) continue;
                if (entity.isArmorStand() || entity.isMinecartChest()) continue; // Skip furniture

                double tx = entity.getX();
                double ty = entity.getY() + (entity.getBoundingBox().maxY - entity.getBoundingBox().minY) / 2.0;
                double tz = entity.getZ();

                double dx = tx - px;
                double dy = ty - py;
                double dz = tz - pz;
                double dist = std::sqrt(dx*dx + dy*dy + dz*dz);

                if (dist > targetRange) continue;

                float yawToTarget = (float)(std::atan2(dz, dx) * 180.0 / 3.14159265358979323846) - 90.0f;
                // Pitch needs distance and height diff
                float pitchToTarget = (float)-(std::atan2(dy, std::sqrt(dx*dx + dz*dz)) * 180.0 / 3.14159265358979323846);

                float yawDiff = std::abs(getAngleDifference(pYaw, yawToTarget));
                
                // If it's within FOV and closer to crosshair than current best
                if (yawDiff < bestFov) {
                    bestFov = yawDiff;
                    bestTarget = &entity;
                }
            }

            if (bestTarget) {
                double tx = bestTarget->getX();
                double ty = bestTarget->getY() + (bestTarget->getBoundingBox().maxY - bestTarget->getBoundingBox().minY) / 2.0;
                double tz = bestTarget->getZ();

                double dx = tx - px;
                double dy = ty - py;
                double dz = tz - pz;

                float yawToTarget = (float)(std::atan2(dz, dx) * 180.0 / 3.14159265358979323846) - 90.0f;
                float pitchToTarget = (float)-(std::atan2(dy, std::sqrt(dx*dx + dz*dz)) * 180.0 / 3.14159265358979323846);

                float yawDiff = getAngleDifference(yawToTarget, pYaw);
                float pitchDiff = getAngleDifference(pitchToTarget, pPitch);

                // Smoothing: lower speed -> lower ratio, smoother snap
                // E.g., speed=10 is instant (1.0). speed=1 is very slow (0.01)
                float smoothFactor = aimSpeed / 10.0f;
                
                // Add some random humanization jitter so linear anti-cheats don't flag perfectly smooth moves
                float jitterYaw = ((rand() % 100) - 50) / 1000.0f;
                float jitterPitch = ((rand() % 100) - 50) / 1000.0f;

                float newYaw = pYaw + (yawDiff * smoothFactor) + jitterYaw;
                float newPitch = pPitch + (pitchDiff * smoothFactor) + jitterPitch;

                player.setYaw(newYaw);
                player.setPitch(newPitch);
            }
            env->DeleteLocalRef(worldObj);
        }
    }

    // ─── 2. Triggerbot & AutoClicker ─────────────────────────────────
    bool shouldClick = false;

    // CPS Limiter / AutoClicker
    if (autoClicker && lmbHeld) {
        long long delay = (long long)(1000.0f / cps);
        // Add random variance between -20% and +20% cps to look legit
        long long variance = (long long)((delay * 0.2f) * ((rand() % 100) - 50) / 50.0f);
        if (nowMs - m_lastClickTime > (delay + variance)) {
            shouldClick = true;
        }
    }

    // Triggerbot logic
    if (triggerbot) {
        static jfieldID s_hitResultField = nullptr;
        static jclass s_entityHitResultClass = nullptr;
        static jclass s_mcClass = nullptr;

        if (!s_mcClass) {
            s_mcClass = JvmWrapper::findClass(Mappings::Minecraft_Class);
            if (s_mcClass) {
                s_hitResultField = env->GetFieldID(s_mcClass, Mappings::MC_hitResult, Mappings::MC_hitResult_Sig);
            }
            s_entityHitResultClass = JvmWrapper::findClass(Mappings::EntityHitResult_Class);
        }

        if (s_hitResultField && s_entityHitResultClass) {
            jobject hitResult = env->GetObjectField(mc, s_hitResultField);
            if (hitResult && !env->ExceptionCheck()) {
                if (env->IsInstanceOf(hitResult, s_entityHitResultClass)) {
                    // Looking at entity in vanilla crosshair
                    long long delay = (long long)(1000.0f / cps);
                    long long variance = (long long)((delay * 0.2f) * ((rand() % 100) - 50) / 50.0f);
                    if (nowMs - m_lastClickTime > (delay + variance)) {
                        shouldClick = true;
                    }
                }
                env->DeleteLocalRef(hitResult);
            } else {
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
        }
    }

    // Simulate the click hardware event
    if (shouldClick) {
        HWND mcHwnd = FindWindowA("GLFW30", nullptr);
        if (mcHwnd && GetForegroundWindow() == mcHwnd) {
            // If the user is holding physically, we MUST release it first to register a new click
            // This forces Glfw to see a new transition.
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            
            // Tiny gap so the game detects it (randomized 2-8ms)
            int gap = 2 + (rand() % 7);
            Sleep(gap); 
            
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            m_lastClickTime = nowMs;
        }
    }

    env->DeleteLocalRef(playerObj);
    env->DeleteLocalRef(mc);
}
