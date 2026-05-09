#pragma once
#include "modules/module.h"
#include <chrono>

class AimAssist : public Module {
public:
    AimAssist();

    void onUpdate(JNIEnv* env) override;
    void onRender() override {} // No rendering needed for AimAssist

private:
    float getAngleDifference(float a, float b);
    float wrapAngleTo180(float angle);

    long long m_lastClickTime = 0;
};
