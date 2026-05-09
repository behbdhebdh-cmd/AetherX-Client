#pragma once
#include "modules/module.h"

class Reach : public Module {
public:
    Reach();
    void onUpdate(JNIEnv* env) override;
    void onDisable() override;

private:
    void setReach(JNIEnv* env, double entityReach, double blockReach);
    void sendPositionPacket(JNIEnv* env, jobject connection, double x, double y, double z, float yaw, float pitch, bool onGround);

    // Cached JNI IDs for packet sending
    static inline bool s_netInit = false;

    // Attribute system
    static inline jclass    s_attributesClass = nullptr;
    static inline jfieldID  s_entityInteractionRange = nullptr;
    static inline jfieldID  s_blockInteractionRange = nullptr;
    static inline jclass    s_livingEntityClass = nullptr;
    static inline jmethodID s_getAttribute = nullptr;
    static inline jclass    s_attributeInstanceClass = nullptr;
    static inline jmethodID s_setBaseValue = nullptr;

    // Network packet system
    static inline jclass    s_mcClass = nullptr;
    static inline jmethodID s_getConnection = nullptr;
    static inline jclass    s_ccpliClass = nullptr;
    static inline jfieldID  s_connectionField = nullptr;
    static inline jclass    s_connectionClass = nullptr;
    static inline jmethodID s_connectionSend = nullptr;
    static inline jclass    s_movePacketPosClass = nullptr;
    static inline jmethodID s_movePacketPosInit = nullptr;

    // Hit result detection
    static inline jfieldID  s_hitResultField = nullptr;
    static inline jclass    s_entityHitResultClass = nullptr;
    static inline jmethodID s_getTargetEntity = nullptr;

    // Player fields
    static inline jclass    s_playerClass = nullptr;
    static inline jmethodID s_playerAttack = nullptr;
    static inline jmethodID s_playerSwing = nullptr;
    static inline jclass    s_interactionHandClass = nullptr;
    static inline jfieldID  s_mainHandField = nullptr;
};
