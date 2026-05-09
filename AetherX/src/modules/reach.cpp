#include "pch.h"
#include "modules/reach.h"
#include "sdk/minecraft.h"
#include "sdk/entity.h"
#include "sdk/world.h"
#include <cmath>

Reach::Reach() : Module("Reach", 'R') {
    m_floatSettings["reach_distance"] = 3.5f;  // Default: slightly beyond vanilla 3.0
}

void Reach::onDisable() {
    JNIEnv* env = JvmWrapper::getEnv();
    if (env) {
        setReach(env, 3.0, 4.5); // Vanilla defaults
    }
}

void Reach::onUpdate(JNIEnv* env) {
    if (!m_enabled) return;

    float reachDist = m_floatSettings["reach_distance"];
    if (reachDist <= 3.0f) {
        setReach(env, 3.0, 4.5);
        return;
    }

    // Keep the attribute modification so the client detects hits at extended range
    setReach(env, reachDist, reachDist + 1.5);

    // ── Blink/TP Reach: Spoof position closer when attacking ────────
    if (!s_netInit) {
        // One-time init of network packet JNI IDs
        s_mcClass = JvmWrapper::findClass(Mappings::Minecraft_Class);
        if (s_mcClass) {
            s_getConnection = env->GetMethodID(s_mcClass, Mappings::MC_getConnection, Mappings::MC_getConnection_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_getConnection = nullptr; }

            s_hitResultField = env->GetFieldID(s_mcClass, Mappings::MC_hitResult, Mappings::MC_hitResult_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_hitResultField = nullptr; }
        }

        s_ccpliClass = JvmWrapper::findClass(Mappings::ClientCommonPacketListenerImpl_Class);
        if (s_ccpliClass) {
            s_connectionField = env->GetFieldID(s_ccpliClass, Mappings::CCPLI_connection, Mappings::CCPLI_connection_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_connectionField = nullptr; }
        }

        s_connectionClass = JvmWrapper::findClass(Mappings::Connection_Class);
        if (s_connectionClass) {
            s_connectionSend = env->GetMethodID(s_connectionClass, Mappings::Connection_send, Mappings::Connection_send_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_connectionSend = nullptr; }
        }

        s_movePacketPosClass = JvmWrapper::findClass(Mappings::ServerboundMovePlayerPacket_Pos_Class);
        if (s_movePacketPosClass) {
            // The signature for 1.21.1 is (DDDZZ)V: x, y, z, onGround, somethingElse (probably horizontalCollision)
            s_movePacketPosInit = env->GetMethodID(s_movePacketPosClass, "<init>", "(DDDZZ)V");
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_movePacketPosInit = nullptr; }
        }

        s_entityHitResultClass = JvmWrapper::findClass(Mappings::EntityHitResult_Class);
        if (s_entityHitResultClass) {
            s_getTargetEntity = env->GetMethodID(s_entityHitResultClass, Mappings::EntityHitResult_getEntity, Mappings::EntityHitResult_getEntity_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_getTargetEntity = nullptr; }
        }

        s_playerClass = JvmWrapper::findClass(Mappings::Player_Class);
        if (s_playerClass) {
            s_playerAttack = env->GetMethodID(s_playerClass, "attack", "(Lnet/minecraft/world/entity/Entity;)V");
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_playerAttack = nullptr; }

            s_playerSwing = env->GetMethodID(s_playerClass, Mappings::Player_swing, Mappings::Player_swing_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_playerSwing = nullptr; }
        }

        s_interactionHandClass = JvmWrapper::findClass(Mappings::InteractionHand_Class);
        if (s_interactionHandClass) {
            s_mainHandField = env->GetStaticFieldID(s_interactionHandClass, Mappings::InteractionHand_MAIN_HAND, Mappings::InteractionHand_MAIN_HAND_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_mainHandField = nullptr; }
        }

        s_netInit = true;
        printf("[Ghost] Reach TP: net init complete (conn=%p send=%p movePkt=%p entityHit=%p attack=%p)\n",
               s_connectionField, s_connectionSend, s_movePacketPosInit, s_getTargetEntity, s_playerAttack);
    }

    // Check if we have all the required IDs
    if (!s_getConnection || !s_connectionField || !s_connectionSend || !s_movePacketPosInit || !s_hitResultField) {
        return; // Can't do TP reach without network access
    }

    // Get the Minecraft instance and check hitResult
    jobject mc = CMinecraft::getInstance();
    if (!mc) return;

    jobject hitResult = env->GetObjectField(mc, s_hitResultField);
    if (!hitResult || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(mc);
        return;
    }

    // Check if hitResult is an EntityHitResult (player is looking at an entity)
    if (!s_entityHitResultClass || !env->IsInstanceOf(hitResult, s_entityHitResultClass)) {
        env->DeleteLocalRef(hitResult);
        env->DeleteLocalRef(mc);
        return;
    }

    // Get the target entity
    jobject targetEntity = env->CallObjectMethod(hitResult, s_getTargetEntity);
    if (!targetEntity || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(hitResult);
        env->DeleteLocalRef(mc);
        return;
    }

    // Get player
    jobject playerObj = CMinecraft::getPlayer();
    if (!playerObj) {
        env->DeleteLocalRef(targetEntity);
        env->DeleteLocalRef(hitResult);
        env->DeleteLocalRef(mc);
        return;
    }

    // Calculate distance between player and target
    CEntity player(playerObj);
    CEntity target(targetEntity);

    double px = player.getX(), py = player.getY(), pz = player.getZ();
    double tx = target.getX(), ty = target.getY(), tz = target.getZ();
    double dx = tx - px, dy = ty - py, dz = tz - pz;
    double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Only do TP reach if the target is beyond vanilla range but within our extended range
    if (dist > 3.0 && dist <= (double)reachDist) {
        // Check if the player is attacking (left click) — we detect this by checking
        // if the attack cooldown just reset (attackStrengthTicker == 0 means ready)
        // We rely on the game's own attack detection via hitResult being set

        bool leftClick = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        static bool wasLeftClick = false;

        if (!leftClick) {
            wasLeftClick = false;
        } else if (leftClick && !wasLeftClick) {
            wasLeftClick = true;

            jobject listener = env->CallObjectMethod(mc, s_getConnection);
            if (listener && !env->ExceptionCheck()) {
                jobject connection = env->GetObjectField(listener, s_connectionField);
                if (connection && !env->ExceptionCheck()) {
                    // Calculate a spoofed position: move along the line from player to target
                    // Stop at 2.9 blocks away from the target (within vanilla reach)
                    double ratio = (dist - 2.9) / dist;
                    double spoofX = px + dx * ratio;
                    double spoofY = py; // Keep Y the same to avoid falling
                    double spoofZ = pz + dz * ratio;

                    float yaw = player.getYaw();
                    float pitch = player.getPitch();

                    // Step 1: Send position packet to spoofed (closer) position
                    sendPositionPacket(env, connection, spoofX, spoofY, spoofZ, yaw, pitch, true);

                    // Step 2: Attack the entity (this sends the attack packet from the spoofed position)
                    if (s_playerAttack) {
                        env->CallVoidMethod(playerObj, s_playerAttack, targetEntity);
                        if (env->ExceptionCheck()) env->ExceptionClear();
                    }

                    // Step 3: Swing arm for visual feedback
                    if (s_playerSwing && s_interactionHandClass && s_mainHandField) {
                        jobject mainHand = env->GetStaticObjectField(s_interactionHandClass, s_mainHandField);
                        if (mainHand) {
                            env->CallVoidMethod(playerObj, s_playerSwing, mainHand);
                            if (env->ExceptionCheck()) env->ExceptionClear();
                            env->DeleteLocalRef(mainHand);
                        }
                    }

                    // Step 4: Send position packet back to real position
                    sendPositionPacket(env, connection, px, py, pz, yaw, pitch, true);

                    env->DeleteLocalRef(connection);
                } else {
                    if (env->ExceptionCheck()) env->ExceptionClear();
                }
                env->DeleteLocalRef(listener);
            } else {
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
        }
    }

    env->DeleteLocalRef(targetEntity);
    env->DeleteLocalRef(playerObj);
    env->DeleteLocalRef(hitResult);
    env->DeleteLocalRef(mc);
}

void Reach::sendPositionPacket(JNIEnv* env, jobject connection, double x, double y, double z, float yaw, float pitch, bool onGround) {
    if (!s_movePacketPosClass || !s_movePacketPosInit || !s_connectionSend) return;

    // constructor takes 3 doubles (x, y, z) and 2 booleans (onGround, horizontalCollision)
    jobject packet = env->NewObject(s_movePacketPosClass, s_movePacketPosInit,
                                     x, y, z, (jboolean)onGround, (jboolean)false);
    if (packet && !env->ExceptionCheck()) {
        env->CallVoidMethod(connection, s_connectionSend, packet);
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(packet);
    } else {
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
}

void Reach::setReach(JNIEnv* env, double entityReach, double blockReach) {
    static bool init = false;
    if (!init) {
        s_attributesClass = JvmWrapper::findClass("net/minecraft/world/entity/ai/attributes/Attributes");
        if (s_attributesClass) {
            s_entityInteractionRange = env->GetStaticFieldID(s_attributesClass, "ENTITY_INTERACTION_RANGE", "Lnet/minecraft/core/Holder;");
            s_blockInteractionRange = env->GetStaticFieldID(s_attributesClass, "BLOCK_INTERACTION_RANGE", "Lnet/minecraft/core/Holder;");
        }

        s_livingEntityClass = JvmWrapper::findClass("net/minecraft/world/entity/LivingEntity");
        if (s_livingEntityClass) {
            s_getAttribute = env->GetMethodID(s_livingEntityClass, "getAttribute", "(Lnet/minecraft/core/Holder;)Lnet/minecraft/world/entity/ai/attributes/AttributeInstance;");
        }

        s_attributeInstanceClass = JvmWrapper::findClass("net/minecraft/world/entity/ai/attributes/AttributeInstance");
        if (s_attributeInstanceClass) {
            s_setBaseValue = env->GetMethodID(s_attributeInstanceClass, "setBaseValue", "(D)V");
        }
        init = true;
    }

    if (!s_attributesClass || !s_livingEntityClass || !s_attributeInstanceClass) return;
    if (!s_entityInteractionRange || !s_blockInteractionRange || !s_getAttribute || !s_setBaseValue) return;

    jobject playerObj = CMinecraft::getPlayer();
    if (!playerObj) return;

    // Set Entity Reach
    jobject entityHolder = env->GetStaticObjectField(s_attributesClass, s_entityInteractionRange);
    if (entityHolder) {
        jobject attrInst = env->CallObjectMethod(playerObj, s_getAttribute, entityHolder);
        if (attrInst) {
            env->CallVoidMethod(attrInst, s_setBaseValue, entityReach);
            env->DeleteLocalRef(attrInst);
        }
        env->DeleteLocalRef(entityHolder);
        JvmWrapper::checkException();
    }

    // Set Block Reach
    jobject blockHolder = env->GetStaticObjectField(s_attributesClass, s_blockInteractionRange);
    if (blockHolder) {
        jobject attrInst = env->CallObjectMethod(playerObj, s_getAttribute, blockHolder);
        if (attrInst) {
            env->CallVoidMethod(attrInst, s_setBaseValue, blockReach);
            env->DeleteLocalRef(attrInst);
        }
        env->DeleteLocalRef(blockHolder);
        JvmWrapper::checkException();
    }
}
