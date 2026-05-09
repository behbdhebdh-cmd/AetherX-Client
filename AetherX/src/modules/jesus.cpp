#include "pch.h"
#include "modules/jesus.h"
#include "sdk/minecraft.h"
#include "sdk/entity.h"
#include "core/jvm_wrapper.h"

Jesus::Jesus() : Module("Jesus", 0) {
    m_boolSettings["solid"] = true;
    m_floatSettings["bounce"] = 0.0f;
}

void Jesus::onUpdate(JNIEnv* env) {
    if (!m_enabled) return;

    jobject mc = CMinecraft::getInstance();
    if (!mc) return;

    jobject playerObj = CMinecraft::getPlayer();
    if (!playerObj) {
        env->DeleteLocalRef(mc);
        return;
    }

    CEntity player(playerObj);

    // Surface Jesus Logic:
    bool solid = m_boolSettings["solid"];
    bool diving = player.isShiftKeyDown();
    bool hittingWall = player.horizontalCollision();

    if (player.isInWater() && solid && !diving) {
        CEntity::Vec3 delta = player.getDeltaMovement();
        
        // If we are hitting a wall (trying to climb out), don't force ground state.
        // This prevents the "push back" from land.
        if (!hittingWall) {
            // If we are sinking, apply a small upward bounce to stay at the surface.
            // This is smoother than hard-coding 0.0 and prevents server freezes.
            if (delta.y < 0.0) {
                delta.y = 0.1; 
            }

            player.setDeltaMovement(delta.x, delta.y, delta.z);
            player.setOnGround(true); // Allow sprinting and land-speed
        }
    }

    env->DeleteLocalRef(playerObj);
    env->DeleteLocalRef(mc);
}
